#include "art/Framework/Core/ModuleMacros.h" 
#include "art/Framework/Core/EDProducer.h"
#include "art/Utilities/make_tool.h" 
#include "larwirecell/Interfaces/MainTool.h"

// fixme: this should be removed once produces<>() is moved to sink components.
#include "lardataobj/RecoBase/Wire.h"

#include <iostream>

using namespace std;

namespace wcls {

    class WireCellToolkit : public art::EDProducer {
    public:
        explicit WireCellToolkit(fhicl::ParameterSet const& pset);
        virtual ~WireCellToolkit();

        void produce(art::Event & evt);
        void reconfigure(fhicl::ParameterSet const& pset);

    private:

        std::unique_ptr<wcls::MainTool> m_wcls;
        
    };
}


wcls::WireCellToolkit::WireCellToolkit(fhicl::ParameterSet const& pset)
    : EDProducer(pset)
{
    this->reconfigure(pset);
}
wcls::WireCellToolkit::~WireCellToolkit()
{
}

void wcls::WireCellToolkit::produce(art::Event & evt)
{
    m_wcls->process(evt);
}

void wcls::WireCellToolkit::reconfigure(fhicl::ParameterSet const& pset)
{
    auto const& wclsPS = pset.get<fhicl::ParameterSet>("wcls_main");
    m_wcls = art::make_tool<wcls::MainTool>(wclsPS);
    if (! m_wcls) {
        throw cet::exception("WireCellToolkit_module") << "Failed to get Art Tool \"wcls_main\"\n";
    }
    m_wcls->produces(this);
}

namespace wcls{
    DEFINE_ART_MODULE(WireCellToolkit)
}
