#include "SimChannelSink.h"

#include "art/Framework/Principal/Event.h"
#include "art/Framework/Core/EDProducer.h"

#include "WireCellIface/IDepo.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/Pimpos.h"
#include "WireCellUtil/Units.h"
//#include "WireCellUtil/Binning.h"
#include "WireCellGen/BinnedDiffusion.h"

#include <algorithm>

// need to ask Brett about this
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
    /*cfg["rng"] = "Noise";
    cfg["start_time"] = 0.0*units::ns;
    cfg["readout_time"] = 5.0*units::ms;
    cfg["tick"] = 0.5*units::ms;
    cfg["nsigma"] = 3.0; */
    return cfg;
}

void SimChannelSink::configure(const WireCell::Configuration& cfg)
{
    const std::string anode_tn = cfg["anode"].asString();
    if (anode_tn.empty()) {
        THROW(ValueError() << errmsg{"SimChannelSink requires an anode plane"});
    }
    m_anode = Factory::find_tn<IAnodePlane>(anode_tn);
    /*
    const std::string rng_tn = cfg["rng"].asString();
    if (rng_tn.empty()) {
        THROW(ValueError() << errmsg{"SimChannelSink requires a noise source"});
    }
    m_rng = Factory::find_tn<IRandom>(rng_tn);

    m_start_time = get(cfg,"readout_time",0.0*units::ns);
    m_readout_time = get(cfg,"readout_time",5.0*units::ms);
    m_tick = get(cfg,"tick",0.5*units::ms);
    m_nsigma = get(cfg,"nsigma",3.0); */
}

void SimChannelSink::produces(art::EDProducer* prod)
{
    assert(prod);

    // !!!!! MODIFY string !!!!!
    prod->produces< std::vector<sim::SimChannel> >("BR_temp");

}

void SimChannelSink::save_as_simchannel(const WireCell::IDepo::pointer& depo){

    for(auto face : m_anode->faces()){ //pointer to IAnodeFace

    //auto boundbox = face->sensitive(); // return a bounding box containing the volume to which this face is sensitive

    //if(boundbox.inside(depo->pos())){

        for(auto plane : face->planes()){ // pointer to IWirePlane
	  		      
	    // grab geometry information to find the nearest wire
	    const Pimpos* pimpos = plane->pimpos();
	    /*
	    Binning tbins(m_readout_time/m_tick, m_start_time, m_start_time+m_readout_time);

	    Gen::BinnedDiffusion bindiff(*pimpos, tbins, m_nsigma, m_rng);

	    const std::pair<double,double> pitchRange = bindiff.pitch_range(m_nsigma);
	    const std::pair<double,double> timeRange = bindiff.time_range(m_nsigma);
	    std::cout<<"pitch range: "<<pitchRange.first<<" to "<<pitchRange.second<<"\n"
		     <<"time range: "<<timeRange.first<<" to "<<timeRange.second<<std::endl;
	  */
	    // find the distance of the point in "pitch coordinate" for the wire plane coordinate system
	    double depo_dist = pimpos->distance(depo->pos());

	    // pass found distance to get bounding pair of wires
	    std::pair<int,int> wire_pair = pimpos->closest(depo_dist);

	    // translate wire to channel number
	    auto& wires = plane->wires(); // IWire::vector indexed by the wire index from Pimpos

	    unsigned int depo_chan = (unsigned int)wires[wire_pair.first]->channel(); //only take the first to start INVESTIGATE THIS AT Pimpos.cxx
	    
	    double xyz[3] = {depo->pos().x(),
			     depo->pos().y(),
			     depo->pos().z()}; // 3D array [cm]

	    double energy = -1.0; // !!!!! TEMPORARY !!!!!
	    
	    int id = -10000;
	    if(depo->prior()){
	      id = depo->prior()->id();
	    }
	    else{
	      id = depo->id();
	    }

	    //sim::SimChannel *sc = new sim::SimChannel(depo_chan);
	    sim::SimChannel sc(depo_chan);

	    sc.AddIonizationElectrons(id, 
				      depo->time(), // correct time with raw waveform
				      depo->charge(),
				      xyz, 
				      energy);
	    m_mapSC[depo_chan].push_back(sc);
	} // plane
    } //face
      
}

void SimChannelSink::visit(art::Event & event)
{
    std::unique_ptr<std::vector<sim::SimChannel> > out(new std::vector<sim::SimChannel>);

    for(auto& m : m_mapSC){
        for(auto& sc : m.second){
	    out->emplace_back(sc);
        }
	m.second.clear();
    }


    // !!!!! MODIFY string !!!!!    
    event.put(std::move(out), "BR_temp");
    m_mapSC.clear();
}

bool SimChannelSink::operator()(const WireCell::IDepo::pointer& indepo,
				WireCell::IDepo::pointer& outdepo)
{
    // set an IDepo based on last visited event
    outdepo = indepo;
    if (indepo) {
        m_depo = indepo;
    }

    save_as_simchannel(m_depo);
    return true;
}



