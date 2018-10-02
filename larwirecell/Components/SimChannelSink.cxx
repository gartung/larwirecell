#include "SimChannelSink.h"

#include "art/Framework/Principal/Event.h"
#include "art/Framework/Core/EDProducer.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"

#include "WireCellGen/BinnedDiffusion.h"
#include "WireCellIface/IDepo.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/Units.h"

#include <algorithm>

WIRECELL_FACTORY(wclsSimChannelSink, wcls::SimChannelSink,
		 wcls::IArtEventVisitor, WireCell::IDepoFilter)

using namespace wcls;
using namespace WireCell;

SimChannelSink::SimChannelSink()
  : m_depo(nullptr)
{
  m_mapSC.clear();
}

SimChannelSink::~SimChannelSink()
{
}

WireCell::Configuration SimChannelSink::default_configuration() const
{
    Configuration cfg;
    cfg["anode"] = "AnodePlane";
    cfg["rng"] = "Random";
    cfg["start_time"] = -1.6*units::ms; 
    cfg["readout_time"] = 4.8*units::ms;
    cfg["tick"] = 0.5*units::us;
    cfg["nsigma"] = 3.0; 
    cfg["drift_speed"] = 1.098*units::mm/units::us;
    cfg["uboone_u_to_rp"] = 94*units::mm;
    cfg["uboone_v_to_rp"] = 97*units::mm;
    cfg["uboone_y_to_rp"] = 100*units::mm;
    cfg["u_time_offset"] = 0.0*units::us;
    cfg["v_time_offset"] = 0.0*units::us;
    cfg["y_time_offset"] = 0.0*units::us;
    cfg["use_energy"] = false;
    return cfg;
}

void SimChannelSink::configure(const WireCell::Configuration& cfg)
{
    const std::string anode_tn = cfg["anode"].asString();
    if (anode_tn.empty()) {
        THROW(ValueError() << errmsg{"SimChannelSink requires an anode plane"});
    }
    m_anode = Factory::find_tn<IAnodePlane>(anode_tn);
    
    const std::string rng_tn = cfg["rng"].asString();
    if (rng_tn.empty()) {
        THROW(ValueError() << errmsg{"SimChannelSink requires a noise source"});
    }
    m_rng = Factory::find_tn<IRandom>(rng_tn);

    m_start_time = get(cfg,"start_time",-1.6*units::ms);
    m_readout_time = get(cfg,"readout_time",4.8*units::ms);
    m_tick = get(cfg,"tick",0.5*units::us);
    m_nsigma = get(cfg,"nsigma",3.0); 
    m_drift_speed = get(cfg,"drift_speed",1.098*units::mm/units::us);
    m_uboone_u_to_rp = get(cfg,"uboone_u_to_rp",94*units::mm);
    m_uboone_v_to_rp = get(cfg,"uboone_v_to_rp",97*units::mm);
    m_uboone_y_to_rp = get(cfg,"uboone_y_to_rp",100*units::mm);
    m_u_time_offset = get(cfg,"u_time_offset",0.0*units::us);
    m_v_time_offset = get(cfg,"v_time_offset",0.0*units::us);
    m_y_time_offset = get(cfg,"y_time_offset",0.0*units::us);
    m_use_energy = get(cfg,"use_energy",false);

    uboone_u = new Pimpos(2400, -3598.5, 3598.5, Point(0,sin(Pi/6),cos(Pi/6)), Point(0,cos(5*Pi/6),sin(5*Pi/6)), Point(94,9.7,5184.98), 1);
    uboone_v = new Pimpos(2400, -3598.5, 3598.5, Point(0,sin(5*Pi/6),cos(5*Pi/6)), Point(0,cos(Pi/6),sin(Pi/6)), Point(94,9.7,5184.98), 1);
    uboone_y = new Pimpos(3456, -5182.5, 5182.5, Point(0,1,0), Point(0,0,1), Point(94,9.7,5184.98), 1);
}

void SimChannelSink::produces(art::EDProducer* prod)
{
    assert(prod);
    prod->produces< std::vector<sim::SimChannel> >("simpleSC");
}

void SimChannelSink::save_as_simchannel(const WireCell::IDepo::pointer& depo){

    if(!depo) return;
    if(depo->charge()<1.0) return;
    //std::cout<<"depo charge: "<<depo->charge()<<std::endl;
    for(auto face : m_anode->faces()){
        auto boundbox = face->sensitive();
        if(!boundbox.inside(depo->pos())) continue;

	int plane = -1;
	for(Pimpos* pimpos : {uboone_u, uboone_v, uboone_y}){
	    plane++;
	    Binning tbins(m_readout_time/m_tick, m_start_time, m_start_time+m_readout_time);
	    Gen::BinnedDiffusion bindiff(*pimpos, tbins, m_nsigma, m_rng);
	    bindiff.add(depo,depo->extent_long() / m_drift_speed, depo->extent_tran());
	    double depo_dist = pimpos->distance(depo->pos());
	    std::pair<int,int> wire_pair = pimpos->closest(depo_dist); 
	    auto& wires = face->plane(plane)->wires();

	    double xyz[3];
	    int id = -10000;
	    double energy = 100.0;
	    if(depo->prior()){ 
	      id = depo->prior()->id(); 
	      //if(m_use_energy == true){ energy = depo->prior()->energy(); }
	    }
	    else{ 
	      id = depo->id(); 
	      //if(m_use_energy == true){ energy = depo->energy(); }
	    }

	    const std::pair<int,int> impact_range = bindiff.impact_bin_range(m_nsigma);
	    int reference_impact = (int)(impact_range.first+impact_range.second)/2;
	    int reference_channel = wires[wire_pair.first]->channel();
	    
	    for(int i=impact_range.first; i<=impact_range.second; i++){
	        auto impact_data = bindiff.impact_data(i);
		if(!impact_data) continue;
		int channel_change = i-reference_impact; // (default) 1 impact/wire region
		//int channel_change = find_uboone_channel(i, reference_impact, wire_pair.second); // 2 impacts/wire region
		
		auto wave = impact_data->waveform();
		const std::pair<double,double> time_span = impact_data->span(m_nsigma);
		const int min_tick = tbins.bin(time_span.first);
		const int max_tick = tbins.bin(time_span.second);
		for(int t=min_tick; t<=max_tick; t++){

		    double tdc = tbins.center(t);
		    if(plane == 0){ 
		        tdc = tdc + (m_uboone_u_to_rp/m_drift_speed) + m_u_time_offset; 
			xyz[0] = depo->pos().x()/units::cm - m_uboone_u_to_rp/units::cm; 
		    }
		    if(plane == 1){
		        tdc = tdc + (m_uboone_v_to_rp/m_drift_speed) + m_v_time_offset; 
			xyz[0] = depo->pos().x()/units::cm - m_uboone_v_to_rp/units::cm; 
		    }
		    if(plane == 2){
		        tdc = tdc + (m_uboone_y_to_rp/m_drift_speed) + m_y_time_offset; 
			xyz[0] = depo->pos().x()/units::cm - m_uboone_y_to_rp/units::cm; 
		    }

		    unsigned int temp_time = (unsigned int) ( (tdc/units::us+4050) / (m_tick/units::us) ); // hacked G4 to TDC
		    double temp_charge = wave[t];
		    if(temp_charge>1){

		        unsigned int depo_chan = (unsigned int)reference_channel+channel_change;
			auto channelData = m_mapSC.find(depo_chan);
			sim::SimChannel& sc = (channelData == m_mapSC.end())
			  ? (m_mapSC[depo_chan]=sim::SimChannel(depo_chan))
			  : channelData->second;
		
			sc.AddIonizationElectrons(id, temp_time, temp_charge, xyz, energy);			
		    }		    
		} // t
	    } // i
	} // plane
    } //face
}

int SimChannelSink::find_uboone_channel(int impact_bin,int reference_bin, int centroid_impact)
{
  int channel_change = 0;
  float num_channel = 1.0;
  float steps = (float)impact_bin - reference_bin;
  float threshold = 0.0;

  if(steps>0 && centroid_impact==0) threshold = 1;
  if(steps>0 && centroid_impact==-1) threshold = 2;
  if(steps<0 && centroid_impact==0) threshold = 2;
  if(steps<0 && centroid_impact==-1) threshold = 1;

  while(num_channel>abs(channel_change)){
    if(abs(steps)/(threshold+(2*(num_channel-1.0))) >= 1){
      channel_change = (int)num_channel*(abs(steps)/steps);
      num_channel++;
    }
    else{ break; }
  }

  return channel_change;
}

void SimChannelSink::visit(art::Event & event)
{
    std::unique_ptr<std::vector<sim::SimChannel> > out(new std::vector<sim::SimChannel>);

    for(auto& m : m_mapSC){
      sim::SimChannel s = m.second;
      out->emplace_back(m.second);
    }
    
    event.put(std::move(out), "simpleSC");
    m_mapSC.clear();
}

bool SimChannelSink::operator()(const WireCell::IDepo::pointer& indepo,
				WireCell::IDepo::pointer& outdepo)
{
    outdepo = indepo;
    if (indepo) {
        m_depo = indepo;
    }

    save_as_simchannel(m_depo);
    return true;
}



