
#include "SimChannelSaver.h"
//#include "art/Framework/Principal/Handle.h" 

#include "art/Framework/Principal/Event.h"
#include "art/Framework/Core/EDProducer.h"

#include "WireCellIface/IDepo.h"
#include "WireCellUtil/NamedFactory.h"

#include <algorithm>

WIRECELL_FACTORY(wclsSimChannelSaver, wcls::SimChannelSaver,
		 wcls::IArtEventVisitor, WireCell::IDepoFilter)


using namespace wcls;
using namespace WireCell;

SimChannelSaver::SimChannelSaver()
    : received_eos(false)
{
}

SimChannelSaver::~SimChannelSaver()
{
}


WireCell::Configuration SimChannelSaver::default_configuration() const
{
    Configuration cfg;
    return cfg;
}

void SimChannelSaver::configure(const WireCell::Configuration& cfg)
{
}

void SimChannelSaver::produces(art::EDProducer* prod)
{
    assert(prod);
}

void SimChannelSaver::create_simchannels(art::Event & event,WireCell::IDepo::vector const& depo_vec)
{
    std::cout << "\tIn event " << event.event() << " with " << depo_vec.size() << " drifted depos." << std::endl;
}

void SimChannelSaver::visit(art::Event & event)
{
    if(!received_eos){
	std::cout << "SCSaver: visiting event without receiving EOS."
		  << " DepVec size is " << in_depo_vec.size()
		  << std::endl;
	return;
    }
    
    if(in_depo_vec.size()==0){
	std::cout << "SCSaver: visiting event but DepoVec size is " << in_depo_vec.size()
		  << std::endl;
    }

    create_simchannels(event,in_depo_vec);

    in_depo_vec.clear();
    received_eos = false;
}

bool SimChannelSaver::operator()(const WireCell::IDepo::pointer& indepo,
				 WireCell::IDepo::pointer& outdepo)
{
    //collect the depos
    outdepo = indepo;
    
    if(indepo){
	in_depo_vec.push_back(indepo);
	received_eos = false;
    }
    else{
	received_eos = true;
    }

    return true;
}

// Local Variables:
// mode: c++
// c-basic-offset: 4
// End:
