#include "SimChannelSink.h"

#include "art/Framework/Principal/Event.h"
#include "art/Framework/Core/EDProducer.h"

#include "WireCellIface/IDepo.h"
#include "WireCellGen/GaussianDiffusion.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/Units.h"

WIRECELL_FACTORY(wclsSimChannelSink, wcls::SimChannelSink,
		 wcls::IArtEventVisitor, WireCell::IDepoFilter)

using namespace wcls;
using namespace WireCell;

SimChannelSink::SimChannelSink()
  : m_depo(nullptr)
{
  m_mapSC.clear();
  //uboone_u = new Pimpos(2400, -3598.5, 3598.5, Point(0,sin(Pi/6),cos(Pi/6)), Point(0,cos(5*Pi/6),sin(5*Pi/6)), Point(94,9.7,5184.98), 1);
  //uboone_v = new Pimpos(2400, -3598.5, 3598.5, Point(0,sin(5*Pi/6),cos(5*Pi/6)), Point(0,cos(Pi/6),sin(Pi/6)), Point(94,9.7,5184.98), 1);
  //uboone_y = new Pimpos(3456, -5182.5, 5182.5, Point(0,1,0), Point(0,0,1), Point(94,9.7,5184.98), 1);
}

SimChannelSink::~SimChannelSink()
{
  delete uboone_u;
  delete uboone_v;
  delete uboone_y;
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

    cfg["u_nwires"] = 2400;
    cfg["v_nwires"] = 2400;
    cfg["y_nwires"] = 3456;
    cfg["u_minwirepitch"] = -3598.5;
    cfg["u_maxwirepitch"] = 3598.5;
    cfg["v_minwirepitch"] = -3598.5;
    cfg["v_maxwirepitch"] = 3598.5;
    cfg["y_minwirepitch"] = -5182.5;
    cfg["y_maxwirepitch"] = 5182.5;

    cfg["u_wirevec_x"] = 0;
    cfg["u_wirevec_y"] = 0.5;
    cfg["u_wirevec_z"] = 0.86602540;
    cfg["u_pitchvec_x"] = 0;
    cfg["u_pitchvec_y"] = -0.86602540;
    cfg["u_pitchvec_z"] = 0.5;
    cfg["u_origvec_x"] = 94;
    cfg["u_origvec_y"] = 9.7;
    cfg["u_origvec_z"] = 5184.98;

    cfg["v_wirevec_x"] = 0;
    cfg["v_wirevec_y"] = 0.5;
    cfg["v_wirevec_z"] = -0.86602540;
    cfg["v_pitchvec_x"] = 0;
    cfg["v_pitchvec_y"] = 0.86602540;
    cfg["v_pitchvec_z"] = 0.5;
    cfg["v_origvec_x"] = 94;
    cfg["v_origvec_y"] = 9.7;
    cfg["v_origvec_z"] = 5184.98;

    cfg["y_wirevec_x"] = 0;
    cfg["y_wirevec_y"] = 1;
    cfg["y_wirevec_z"] = 0;
    cfg["y_pitchvec_x"] = 0;
    cfg["y_pitchvec_y"] = 0;
    cfg["y_pitchvec_z"] = 1;
    cfg["y_origvec_x"] = 94;
    cfg["y_origvec_y"] = 9.7;
    cfg["y_origvec_z"] = 5184.98;

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

    m_u_nwires = get(cfg,"u_nwires",2400);
    m_v_nwires = get(cfg,"v_wires",2400);
    m_y_nwires = get(cfg,"y_nwires",3456);
    m_u_minwirepitch = get(cfg,"u_minwirepitch",-3598.5);
    m_u_maxwirepitch = get(cfg,"u_maxwirepitch",3598.5);
    m_v_minwirepitch = get(cfg,"v_minwirepitch",-3598.5);
    m_v_maxwirepitch = get(cfg,"v_maxwirepitch",3598.5);
    m_y_minwirepitch = get(cfg,"y_minwirepitch",-5182.5);
    m_y_maxwirepitch = get(cfg,"y_maxwirepitch",5182.5);

    m_u_wirevec_x = get(cfg,"u_wirevec_x",0);
    m_u_wirevec_y = get(cfg,"u_wirevec_y",0.5);
    m_u_wirevec_z = get(cfg,"u_wirevec_z",0.86602540);
    m_u_pitchvec_x = get(cfg,"u_pitchvec_x",0);
    m_u_pitchvec_y = get(cfg,"u_pitchvec_y",-0.86602540);
    m_u_pitchvec_z = get(cfg,"u_pitchvec_z",0.5);
    m_u_origvec_x = get(cfg,"u_origvec_x",94);
    m_u_origvec_y = get(cfg,"u_origvec_y",9.7);
    m_u_origvec_z = get(cfg,"u_origvec_z",5184.98);

    m_v_wirevec_x = get(cfg,"v_wirevec_x",0);
    m_v_wirevec_y = get(cfg,"v_wirevec_y",0.5);
    m_v_wirevec_z = get(cfg,"v_wirevec_z",-0.86602540);
    m_v_pitchvec_x = get(cfg,"v_pitchvec_x",0);
    m_v_pitchvec_y = get(cfg,"v_pitchvec_y",0.86602540);
    m_v_pitchvec_z = get(cfg,"v_pitchvec_z",0.5);
    m_v_origvec_x = get(cfg,"v_origvec_x",94);
    m_v_origvec_y = get(cfg,"v_origvec_y",9.7);
    m_v_origvec_z = get(cfg,"v_origvec_z",5184.98);

    m_y_wirevec_x = get(cfg,"y_wirevec_x",0);
    m_y_wirevec_y = get(cfg,"y_wirevec_y",1);
    m_y_wirevec_z = get(cfg,"y_wirevec_z",0);
    m_y_pitchvec_x = get(cfg,"y_pitchvec_x",0);
    m_y_pitchvec_y = get(cfg,"y_pitchvec_y",0);
    m_y_pitchvec_z = get(cfg,"y_pitchvec_z",1);
    m_y_origvec_x = get(cfg,"y_origvec_x",94);
    m_y_origvec_y = get(cfg,"y_origvec_y",9.7);
    m_y_origvec_z = get(cfg,"y_origvec_z",5184.98);

    //define anode planes using configuration
    uboone_u = new Pimpos(m_u_nwires, m_u_minwirepitch, m_u_maxwirepitch, 
                          Point(m_u_wirevec_x,m_u_wirevec_y,m_u_wirevec_z), 
                          Point(m_u_pitchvec_x,m_u_pitchvec_y,m_u_pitchvec_z), 
                          Point(m_u_origvec_x,m_u_origvec_y,m_u_origvec_z),
                          1);
    uboone_v = new Pimpos(m_v_nwires, m_v_minwirepitch, m_v_maxwirepitch,
                          Point(m_v_wirevec_x,m_v_wirevec_y,m_v_wirevec_z),
                          Point(m_v_pitchvec_x,m_v_pitchvec_y,m_v_pitchvec_z),
                          Point(m_v_origvec_x,m_v_origvec_y,m_v_origvec_z), 
                          1);
    uboone_y = new Pimpos(m_y_nwires, m_y_minwirepitch, m_y_maxwirepitch, 
                          Point(m_y_wirevec_x,m_y_wirevec_y,m_y_wirevec_z),
                          Point(m_y_pitchvec_x,m_y_pitchvec_y,m_y_pitchvec_z),
                          Point(m_y_origvec_x,m_y_origvec_y,m_y_origvec_z),
                          1);

}

void SimChannelSink::produces(art::EDProducer* prod)
{
    assert(prod);
    prod->produces< std::vector<sim::SimChannel> >("simpleSC");
}

void SimChannelSink::save_as_simchannel(const WireCell::IDepo::pointer& depo){
  Binning tbins(m_readout_time/m_tick, m_start_time, m_start_time+m_readout_time);

  //check
  //std::cout << "SIM CHANNEL SINK HERE" << std::endl;

  if(!depo) return;

  int ctr = 0;
  while(ctr<1){
    ctr++;
    //      if(ctr % 10000==0){ std::cout<<"COUNTER "<<ctr<<std::endl;}
    for(auto face : m_anode->faces()){
      auto boundbox = face->sensitive();
      if(!boundbox.inside(depo->pos())) continue;

      int plane = -1;
      for(Pimpos* pimpos : {uboone_u, uboone_v, uboone_y}){
        plane++;

	const double center_time = depo->time();
	const double center_pitch = pimpos->distance(depo->pos());

	Gen::GausDesc time_desc(center_time, depo->extent_long() / m_drift_speed);
	{
	  double nmin_sigma = time_desc.distance(tbins.min());
	  double nmax_sigma = time_desc.distance(tbins.max());

	  double eff_nsigma = depo->extent_long() / m_drift_speed>0?m_nsigma:0;
	  if (nmin_sigma > eff_nsigma || nmax_sigma < -eff_nsigma) {
	    break;
	  }
	}

	auto ibins = pimpos->impact_binning();

	Gen::GausDesc pitch_desc(center_pitch, depo->extent_tran());
	{
	  double nmin_sigma = pitch_desc.distance(ibins.min());
	  double nmax_sigma = pitch_desc.distance(ibins.max());

	  double eff_nsigma = depo->extent_tran()>0?m_nsigma:0;
	  if (nmin_sigma > eff_nsigma || nmax_sigma < -eff_nsigma) {
            break;
	  }
	}

	auto gd = std::make_shared<Gen::GaussianDiffusion>(depo, time_desc, pitch_desc);
	gd->set_sampling(tbins, ibins, m_nsigma, 0, 1);

	double xyz[3];
	int id = -10000;
	double energy = 100.0;
	if(depo->prior()){
	  id = depo->prior()->id();
	  if(m_use_energy){ energy = depo->prior()->energy(); }
	}
	else{
	  id = depo->id();
	  if(m_use_energy){ energy = depo->energy(); }
	}

	const auto patch = gd->patch();
	const int poffset_bin = gd->poffset_bin();
	const int toffset_bin = gd->toffset_bin();
	const int np = patch.rows();
	const int nt = patch.cols();

	int min_imp = 0;
	int max_imp = ibins.nbins();

	for (int pbin = 0; pbin != np; pbin++){
	  int abs_pbin = pbin + poffset_bin;
	  if (abs_pbin < min_imp || abs_pbin >= max_imp) continue;

	  int channel = abs_pbin;
	  if(plane == 1){ channel = abs_pbin+m_u_nwires; }
	  if(plane == 2){ channel = abs_pbin+m_u_nwires+m_v_nwires; }

	  auto channelData = m_mapSC.find(channel);
	  sim::SimChannel& sc = (channelData == m_mapSC.end())
	    ? (m_mapSC[channel]=sim::SimChannel(channel))
	    : channelData->second;

	  for (int tbin = 0; tbin!= nt; tbin++){
	    int abs_tbin = tbin + toffset_bin;
	    double charge = patch(pbin, tbin);
	    double tdc = tbins.center(abs_tbin);

	    if(plane == 0){
	      tdc = tdc + (m_uboone_u_to_rp/m_drift_speed) + m_u_time_offset;
	      xyz[0] = depo->pos().x()/units::cm - 94*units::mm/units::cm; //m_uboone_u_to_rp/units::cm;
	    }
	    if(plane == 1){
	      tdc = tdc + (m_uboone_v_to_rp/m_drift_speed) + m_v_time_offset;
	      xyz[0] = depo->pos().x()/units::cm - 97*units::mm/units::cm; //m_uboone_v_to_rp/units::cm;
	    }
	    if(plane == 2){
	      tdc = tdc + (m_uboone_y_to_rp/m_drift_speed) + m_y_time_offset;
	      xyz[0] = depo->pos().x()/units::cm - 100*units::mm/units::cm; //m_uboone_y_to_rp/units::cm;
	    }
	    xyz[1] = depo->pos().y()/units::cm;
	    xyz[2] = depo->pos().z()/units::cm;

	    unsigned int temp_time = (unsigned int) ( (tdc/units::us+4050) / (m_tick/units::us) ); // hacked G4 to TDC
	    charge = abs(charge);
	    if(charge>1){
	      sc.AddIonizationElectrons(id, temp_time, charge, xyz, energy*abs(charge/depo->charge()));
	    }
	  }
	}
      } // plane
    } //face
  }
}

void SimChannelSink::visit(art::Event & event)
{
    std::unique_ptr<std::vector<sim::SimChannel> > out(new std::vector<sim::SimChannel>);

    for(auto& m : m_mapSC){
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
	save_as_simchannel(m_depo);
    }

    return true;
}
