
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

void SimChannelSaver::print_depo(WireCell::IDepo::pointer const& depo_ptr)
{
    std::cout << "(x,y,z)=(" 
	      << depo_ptr->pos().x() << ","
	      << depo_ptr->pos().y() << ","
	      << depo_ptr->pos().z() << ")\n"

	      << "extent (long,trans)=("
	      << depo_ptr->extent_long() << ","
	      << depo_ptr->extent_tran() << ")\n"

	      << "charge=" << depo_ptr->charge() << "\n"
	
	      << "id=" << depo_ptr->id() << "\n"
	      << "pdg=" << depo_ptr->pdg() << "\n"

	      << "prior?" << (depo_ptr->prior()!=nullptr) << "\n"
	      << std::endl;

    if(depo_ptr->prior()){
	std::cout << "Prior..." << std::endl;
	print_depo(depo_ptr->prior());
    }
}

void SimChannelSaver::create_simchannels(art::Event & event,WireCell::IDepo::vector const& depo_vec)
{
    std::cout << "In event " << event.event() << " with " << depo_vec.size() << " drifted depos." << std::endl;

    std::cout << "\t\tFirst depo..." << std::endl;

    for(size_t i=0; i<10; ++i){
	if (i>depo_vec.size()) break;
	print_depo(depo_vec[i]);
    }
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
