#include "SimChannelSink.h"

#include "art/Framework/Principal/Event.h"
#include "art/Framework/Core/EDProducer.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"

#include "WireCellGen/BinnedDiffusion.h"
#include "WireCellIface/IDepo.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/Pimpos.h"
#include "WireCellUtil/Units.h"


#include "lardata/DetectorInfoServices/DetectorClocksServiceStandard.h"

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
    cfg["start_time"] = 0.0*units::ns;
    cfg["readout_time"] = 5.0*units::ms;
    cfg["tick"] = 0.5*units::us;
    cfg["nsigma"] = 3.0; 
    cfg["drift_speed"] = 1.1*units::mm/units::us;
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

    m_start_time = get(cfg,"start_time",0.0*units::ns);
    m_readout_time = get(cfg,"readout_time",5.0*units::ms);
    m_tick = get(cfg,"tick",0.5*units::us);
    m_nsigma = get(cfg,"nsigma",3.0); 
    m_drift_speed = get(cfg,"drift_speed",1.110*units::mm/units::us);
}

void SimChannelSink::produces(art::EDProducer* prod)
{
    assert(prod);

    prod->produces< std::vector<sim::SimChannel> >("simpleSC");
}

void SimChannelSink::save_as_simchannel(const WireCell::IDepo::pointer& depo){

  //art::ServiceHandle<detinfo::DetectorClocksServiceStandard> tss;
  //auto const* ts = tss->provider();
    
    if(!depo) return;

    for(auto face : m_anode->faces()){
        auto boundbox = face->sensitive();
	if(!boundbox.inside(depo->pos())) continue;

	for(auto plane : face->planes()){
	  const Pimpos* pimpos = plane->pimpos();
	  Binning tbins(m_readout_time/m_tick, m_start_time, m_start_time+m_readout_time);
	  Gen::BinnedDiffusion bindiff(*pimpos, tbins, m_nsigma, m_rng);
	  bindiff.add(depo,depo->extent_long() / m_drift_speed, depo->extent_tran());
	  double depo_dist = pimpos->distance(depo->pos());
	  std::pair<int,int> wire_pair = pimpos->closest(depo_dist); 
	  auto& wires = plane->wires();

	  double xyz[3] = {depo->pos().x()/units::cm,
			   depo->pos().y()/units::cm,
			   depo->pos().z()/units::cm}; 
	  double energy = 200.0;
	  int id = -10000;
	  if(depo->prior()){ id = depo->prior()->id(); }
	  else{ id = depo->id(); }
	
	  //unsigned int temp_time = (unsigned int) (depo->time()/units::us-(-4.05e3)-4050+1.6e3) / (m_tick/units::us); // hacked G4 to tick
	  //unsigned int temp_time = (unsigned int) (depo->time()/units::us-(-4.05e3) / (m_tick/units::us)); // hacked G4 to TDC
	  //unsigned int temp_time = (unsigned int) depo->time()/units::ns; // G4
	  //auto tdc = ts->TPCG4Time2TDC(temp_time);
	  
	  const std::pair<int,int> impact_range = bindiff.impact_bin_range(m_nsigma);
	  int reference_impact = (int)(impact_range.first+impact_range.second)/2;
	  int reference_channel = wires[wire_pair.first]->channel();

	  for(int i=impact_range.first; i<=impact_range.second; i++){
	    auto impact_data = bindiff.impact_data(i);
	    if(!impact_data) continue;
		      
	    auto wave = impact_data->waveform();
	    const std::pair<double,double> time_span = impact_data->span(m_nsigma);
	    const int min_tick = tbins.bin(time_span.first);
	    const int max_tick = tbins.bin(time_span.second);
	    for(int t=min_tick; t<=max_tick; t++){

	      const double tdc = tbins.center(t);
	      unsigned int temp_time = (unsigned int) (tdc/units::us-(-4.05e3)-4050+1.6e3) / (m_tick/units::us); // hacked G4 to tick

	      //double temp_charge = depo->charge();		    
	      double temp_charge = wave[t];

	      if(temp_charge>1){

		int channel_change = find_uboone_channel(i,reference_impact,wire_pair.second);
		unsigned int depo_chan = (unsigned int)reference_channel+channel_change;

		auto channelData = m_mapSC.find(depo_chan);
		sim::SimChannel& sc = (channelData == m_mapSC.end())
		  ? (m_mapSC[depo_chan]=sim::SimChannel(depo_chan))
		  : channelData->second;
		
		sc.AddIonizationElectrons(id, 
					  temp_time,
					  temp_charge, //-wave[t],
					  xyz, 
					  energy);			
	      }

	    } // t
	  } // i
	} // plane
    } //face
      
}

int SimChannelSink::find_uboone_channel(int this_impact_bin, int reference_impact_bin, int centroid_impact)
{
  int channel_change = 0;
  float num_channel = 1.0;
  float steps = (float)this_impact_bin - reference_impact_bin;
  float threshold = 0.0;

  if(steps>0 && centroid_impact>0){ threshold = 6-centroid_impact; }
  if(steps>0 && centroid_impact<0){ threshold = 10+centroid_impact; }
  if(steps<0 && centroid_impact>0){ threshold = 10-centroid_impact; }
  if(steps<0 && centroid_impact<0){ threshold = 6+centroid_impact; }

  while(num_channel>abs(channel_change)){
    if(abs(steps)/(threshold+(10*(num_channel-1.0))) >= 1.0){
      channel_change = (int)num_channel*(abs(steps)/steps);
      num_channel++;
    }
    else{ 
      break;
    }
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



