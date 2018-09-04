/** A WCT component that is a sink of sim::SimChannel
 * (converted from WireCell::IDepo) into an art::Event.
 *
 * Contact brussell@yale.edu for comments/questions.
 */

#ifndef LARWIRECELL_COMPONENTS_SIMCHANNELSINK
#define LARWIRECELL_COMPONENTS_SIMCHANNELSINK

#include "WireCellIface/IDepoFilter.h"
#include "WireCellIface/IConfigurable.h"
#include "WireCellIface/IAnodePlane.h"
#include "WireCellIface/IRandom.h"
#include "larwirecell/Interfaces/IArtEventVisitor.h"
#include "lardataobj/Simulation/SimChannel.h"

namespace wcls {

    class SimChannelSink : public IArtEventVisitor,
                           public WireCell::IDepoFilter,
                           public WireCell::IConfigurable {

    public: 
        SimChannelSink();
	virtual ~SimChannelSink();

	/// IArtEventVisitor
	virtual void produces(art::EDProducer* prod);
	virtual void visit(art::Event & event);

	/// IDepoFilter
	virtual bool operator()(const WireCell::IDepo::pointer& indepo,
				WireCell::IDepo::pointer& outdepo);

	/// IConfigurable
	virtual WireCell::Configuration default_configuration() const;
	virtual void configure(const WireCell::Configuration& config);

    private:
	WireCell::IDepo::pointer m_depo;
	WireCell::IAnodePlane::pointer m_anode;
	WireCell::IRandom::pointer m_rng;

	std::map<unsigned int,sim::SimChannel> m_mapSC;

	void save_as_simchannel(const WireCell::IDepo::pointer& depo);

	double m_readout_time;
	double m_tick;
	double m_start_time;
	double m_nsigma; 
	double m_drift_speed;
  };
}

#endif
