#include "art/Framework/Core/ModuleMacros.h" 
#include "art/Framework/Core/EDProducer.h"

#include "lardataobj/Simulation/SimEnergyDeposit.h"

namespace bogoblip {
    struct BlipMakerConfig {
    };
    class BlipMaker : public art::EDProducer {
    public:

	using Parameters = art::EDProducer::Table<BlipMakerConfig>;

        explicit BlipMaker(Parameters const& params);
        virtual ~BlipMaker();

        void produce(art::Event & evt);
    
    private:

	const BlipMakerConfig m_cfg;
        int m_count;
    };
}

#include <string>
const std::string label = "bogus"; // fixme: make configurable

bogoblip::BlipMaker::BlipMaker(Parameters const& params)
    : m_cfg(params())
    , m_count(0)
{
    produces< std::vector<sim::SimEnergyDeposit> >(label);
}
bogoblip::BlipMaker::~BlipMaker()
{
}
void bogoblip::BlipMaker::produce(art::Event & event)
{
    ++m_count;

    auto out = std::make_unique< std::vector<sim::SimEnergyDeposit> >();
    
    int nphotons = 0;
    const  int nelepercm = 50000;
    const double mevpercm = 2.0;

    double t0 = 0.;
    double t1 = 0.;
    int trackid = 0;
    //int pdgid = 0;

    
    // implicit units are cm, ns and MeV.
    sim::SimEnergyDeposit::Point_t start = {100.,0.,0.};
    sim::SimEnergyDeposit::Point_t end = {150.,10.,50.};
    const auto vdiff = end-start;
    const auto vlen = sqrt(vdiff.Mag2());
    const auto vdir = vdiff.unit();

    const double stepsize = 0.1; // cm
    const int nsteps = vlen/stepsize;

    // larsoft works in ns
    const double ns = 1.0;
    const double us = 1000.0*ns;
    const double ms = 1000.0*us;

    // MB: WCT sim should cut the first, the second should just be on
    // the edge of time acceptance.  The last two bracket the BNB beam
    // gate.
    for (double jump : { -1.6*ms, -1*ms, +3125*ns, (3125+1600)*ns, }) {
        sim::SimEnergyDeposit::Point_t last = start;
        for (int istep=1; istep<nsteps; ++istep) {
            const sim::SimEnergyDeposit::Point_t next(start + stepsize*istep*vdir);
            //std::cerr << last << " -> " << next << "\n";
            out->emplace_back(sim::SimEnergyDeposit(nphotons,
                                                    stepsize * nelepercm,
                                                    stepsize * mevpercm,
                                                    last, next,
                                                    jump + t0,
                                                    jump + t1, trackid));
            last = next;
        }

    }

    std::cerr << "BlipMaker making " << out->size() << " depos to label: " << label << std::endl;


    event.put(std::move(out), label);
}


namespace bogoblip {
    DEFINE_ART_MODULE(BlipMaker)
}

