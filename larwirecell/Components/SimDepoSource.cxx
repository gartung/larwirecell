/** The WC/LS sim depo source adapts larsoft SimEnergyDeposit to WCT's IDepos.  

    Note, this WC/LS file unusually verbose.  Besides just data
    conversion, additional WCT guts are exposed allow optional
    application of WCT ionization/recombination models, or to rely on
    the depo provider to already provide depo in units of #electrons.
*/

#include "SimDepoSource.h"

#include "art/Framework/Principal/Handle.h" 
#include "art/Framework/Principal/Event.h"

#include "lardataobj/Simulation/SimEnergyDeposit.h"

#include "WireCellUtil/Units.h"
#include "WireCellUtil/String.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellIface/SimpleDepo.h"
#include "WireCellIface/IRecombinationModel.h"

WIRECELL_FACTORY(wclsSimDepoSource, wcls::SimDepoSource, wcls::IArtEventVisitor,
                 WireCell::IDepoSource, WireCell::IConfigurable)


namespace units = WireCell::units;

namespace wcls {
    namespace bits {

        // There is more than one way to make ionization electrons.
        // These adapters erase these differences.
        class DepoAdapter {
        public:
            virtual ~DepoAdapter() {}
            virtual double operator()(const sim::SimEnergyDeposit& sed) const = 0;
        };

        // This takes number of electrons directly, and applies a
        // multiplicative scale.
        class ElectronsAdapter : public DepoAdapter {
            double m_scale;
        public:
            ElectronsAdapter(double scale=1.0) : m_scale(scale) {}
            virtual ~ElectronsAdapter() {};
            virtual double operator()(const sim::SimEnergyDeposit& sed) const {
                return m_scale * sed.NumElectrons();
            }
        };
        
        // This one takes a recombination model which only requires dE
        // (ie, assumes MIP).
        class PointAdapter : public DepoAdapter {
            WireCell::IRecombinationModel::pointer m_model;
            double m_scale;
        public:
            PointAdapter(WireCell::IRecombinationModel::pointer model, double scale=1.0)
                : m_model(model), m_scale(scale) {}
            virtual ~PointAdapter() {}
            virtual double operator()(const sim::SimEnergyDeposit& sed) const {
                const double dE = sed.Energy()*units::MeV;
                return m_scale * (*m_model)(dE);
            }
        };

        // This one takes a recombination which is a function of both dE and dX.
        class StepAdapter : public DepoAdapter {
            WireCell::IRecombinationModel::pointer m_model;
            double m_scale;
        public:
            StepAdapter(WireCell::IRecombinationModel::pointer model, double scale=1.0)
                : m_model(model), m_scale(scale) {}
            virtual ~StepAdapter() {}
            virtual double operator()(const sim::SimEnergyDeposit& sed) const {
                const double dE = sed.Energy()*units::MeV;
                const double dX = sed.StepLength()*units::cm;
                return m_scale * (*m_model)(dE, dX);
            }
        };
    }
}


using namespace wcls;

SimDepoSource::SimDepoSource()
    : m_adapter(nullptr)
{
}

SimDepoSource::~SimDepoSource()
{
    if (m_adapter) {
        delete m_adapter;
        m_adapter = nullptr;
    }
}

WireCell::Configuration SimDepoSource::default_configuration() const
{
    WireCell::Configuration cfg;
    
    // An empty model or the special model "electrons" means to take
    // precalcualted input NumElectrons.  Anything else means some
    // recombination model is used.
    cfg["model"] = "";
    // Multiply this number to the number of electrons before forming
    // a WC depo.
    cfg["scale"] = 1.0;
    
    // For locating input in the art::Event
    cfg["art_tag"] = "";

    return cfg;
}
void SimDepoSource::configure(const WireCell::Configuration& cfg)
{
    if (m_adapter) {
        delete m_adapter;
        m_adapter = nullptr;
    }
    double scale = WireCell::get(cfg, "scale", 1.0);

    std::string model_tn = WireCell::get<std::string>(cfg, "model", "");
    std::string model_type = "";
    if (!model_tn.empty()) {
        WireCell::String::split(model_tn)[0];
    }

    if (model_type == "" or model_type == "electrons") {
        m_adapter = new wcls::bits::ElectronsAdapter(scale);
    }
    else {
        auto model = WireCell::Factory::lookup_tn<WireCell::IRecombinationModel>(model_tn);
        if (!model) {
            std::cerr << "wcls::SimDepoSource: unknown recombination model: \"" << model_tn << "\"\n";
            return;             // I should throw something here!
        }
        if (model_type == "MipRecombination") {
            m_adapter = new wcls::bits::PointAdapter(model, scale);
        }
        if (model_type == "BirksRecombination" || model_type == "BoxRecombination") {
            m_adapter = new wcls::bits::StepAdapter(model, scale);
        }
    }
    
    m_inputTag = cfg["art_tag"].asString();
}


void SimDepoSource::visit(art::Event & event)
{
    art::Handle< std::vector<sim::SimEnergyDeposit> > sedvh;
    
    bool okay = event.getByLabel(m_inputTag, sedvh);
    if (!okay || sedvh->empty()) {
        std::string msg = "SimDepoSource failed to get sim::SimEnergyDeposit from art tag: " + m_inputTag.encode();
        std::cerr << msg << std::endl;
        THROW(WireCell::RuntimeError() << WireCell::errmsg{msg});
    }
    
    const size_t ndepos = sedvh->size();
    
    std::cerr << "SimDepoSource got " << ndepos
              << " depos from art tag \"" << m_inputTag
              << "\" returns: " << (okay ? "okay" : "fail") << std::endl;
    
    if (!m_depos.empty()) {
        std::cerr << "SimDepoSource dropping " << m_depos.size()
                  << " unused, prior depos\n";
        m_depos.clear();
    }

    for (size_t ind=0; ind<ndepos; ++ind) {
        auto const& sed = sedvh->at(ind);
        auto pt = sed.MidPoint();
        const WireCell::Point wpt(pt.x()*units::cm, pt.y()*units::cm, pt.z()*units::cm);
        double wt = sed.Time()*units::ns;
        double wq = (*m_adapter)(sed);
        int wid = sed.TrackID();
	//double we = sed.Energy()*units::MeV;

        WireCell::IDepo::pointer depo
            = std::make_shared<WireCell::SimpleDepo>(wt, wpt, wq, nullptr, 0.0, 0.0, wid);
	//= std::make_shared<WireCell::SimpleDepo>(wt, wpt, wq, nullptr, 0.0, 0.0, wid, we);
        m_depos.push_back(depo);
        // std::cerr << ind << ": t=" << wt/units::us << "us,"
        //           << " r=" << wpt/units::cm << "cm, "
        //           << " q=" << wq << "\n";
    }
    // don't trust user to honor time ordering.
    std::sort(m_depos.begin(), m_depos.end(), WireCell::ascending_time);
    std::cerr << "SimDepoSource: ready with " << m_depos.size() << " depos spanning: ["
              << m_depos.front()->time()/units::us << ", "
              << m_depos.back()->time()/units::us << "]us\n";
    m_depos.push_back(nullptr); // EOS marker
}

bool SimDepoSource::operator()(WireCell::IDepo::pointer& out)
{
    if (m_depos.empty()) {
        return false;
    }

    out = m_depos.front();
    m_depos.pop_front();

    // if (!out) {
    //     std::cerr << "SimDepoSource: reached EOS\n";
    // }
    // else {
    //     std::cerr << "SimDepoSource: t=" << out->time()/units::us << "us,"
    //               << " r=" << out->pos()/units::cm << "cm, " << " q=" << out->charge() << "\n";
    // }
    return true;
}
