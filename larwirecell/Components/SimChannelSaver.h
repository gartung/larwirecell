/** A WCT component which passes through IDepos untouched and saves
 * their content as a vector of SimChannels
 * 
 * Aug29, 2018: Wes started this following guidelines from Brett and Jyoit
*/

#ifndef LARWIRECELL_COMPONENTS_SIMCHANNELSAVER
#define LARWIRECELL_COMPONENTS_SIMCHANNELSAVER

#include "WireCellIface/IDepoFilter.h"
#include "WireCellIface/IConfigurable.h"
#include "larwirecell/Interfaces/IArtEventVisitor.h"

#include <string>
#include <vector>
#include <map>

namespace wcls {
    class SimChannelSaver : public IArtEventVisitor, 
                            public WireCell::IDepoFilter,
                            public WireCell::IConfigurable {
    public:
        SimChannelSaver();
        virtual ~SimChannelSaver();

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
        WireCell::IDepo::vector in_depo_vec;
	bool received_eos;

	void print_depo(WireCell::IDepo::pointer const&);
	void create_simchannels(art::Event & event,WireCell::IDepo::vector const&);
    };
}

#endif
