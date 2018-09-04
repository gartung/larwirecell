
#include "FrameSaver.h"
//#include "art/Framework/Principal/Handle.h" 

#include "lardataobj/RecoBase/Wire.h"
#include "lardataobj/RawData/RawDigit.h"

// it would be nice to remove this dependency since it is needed only
// to add bogus information to raw::RawDigit.  
#include "larevt/CalibrationDBI/Interface/DetPedestalService.h"
#include "larevt/CalibrationDBI/Interface/DetPedestalProvider.h"


#include "art/Framework/Principal/Event.h"
#include "art/Framework/Core/EDProducer.h"

#include "WireCellIface/IFrame.h"
#include "WireCellIface/ITrace.h"
#include "WireCellUtil/NamedFactory.h"

#include <algorithm>

WIRECELL_FACTORY(wclsFrameSaver, wcls::FrameSaver,
		 wcls::IArtEventVisitor, WireCell::IFrameFilter)


using namespace wcls;
using namespace WireCell;

FrameSaver::FrameSaver()
    : m_frame(nullptr)
    , m_nticks(0)
{
}

FrameSaver::~FrameSaver()
{
}


WireCell::Configuration FrameSaver::default_configuration() const
{
    Configuration cfg;
    cfg["anode"] = "AnodePlane"; 

    // If true, truncate frame waveforms and save as raw::RawDigit,
    // else leave as floating point recob::Wire
    cfg["digitize"] = false;

    // If true, break full waveforms into smaller ones by removing
    // zeros, otherwise leave waveforms as is.  This option is only
    // applicable if saving to recob::Wire (digitize==false).
    cfg["sparse"] = true;

    // If digitize, raw::RawDigit has slots for pedestal mean and
    // sigma.  Legacy/obsolete code stuff unrelated values into these
    // slots.  If pedestal_mean is a number, it will be stuffed.  If
    // it is the string "fiction" then the DetPedestalService will be
    // used to set some unrelated pedestal value.
    cfg["pedestal_mean"] = 0.0;
    // Stuff some value into pedestal sigma.  This value has nothing
    // to do with the produced NF'ed waveforms.
    cfg["pedestal_sigma"] = 0.0;

    // frames to output, if any
    cfg["frame_tags"] = Json::arrayValue;
    cfg["frame_scale"] = 1.0; 	// multiply this number to all
				// waveform samples.  If list, then
				// one number per frame tags.
    cfg["nticks"] = m_nticks; // if nonzero, force number of ticks in output waveforms. 

    // Summaries to output, if any
    cfg["summary_tags"] = Json::arrayValue;
    cfg["summary_scale"] = 1.0;

    // Names of channel mask maps to save, if any.
    cfg["chanmaskmaps"] = Json::arrayValue;

    return cfg;
}

void FrameSaver::configure(const WireCell::Configuration& cfg)
{
    const std::string anode_tn = cfg["anode"].asString();
    if (anode_tn.empty()) {
        THROW(ValueError() << errmsg{"FrameSaver requires an anode plane"});
    }
    m_anode = Factory::find_tn<IAnodePlane>(anode_tn);

    m_digitize = get(cfg, "digitize", false);
    m_sparse = get(cfg, "sparse", true);

    m_cmms = cfg["chanmaskmaps"];

    m_pedestal_mean = cfg["pedestal_mean"];
    m_pedestal_sigma = get(cfg, "pedestal_sigma", 0.0);

    if (!cfg["frame_scale"].isNull()) {
	m_frame_scale.clear();
	auto jscale = cfg["frame_scale"];
	m_frame_tags.clear();
	auto jtags = cfg["frame_tags"];
	for (auto jtag : jtags) {
	    std::string tag = jtag.asString();

	    // get any waveform scaling for this frame
	    const int ind = m_frame_tags.size();
	    double scale = 1.0;
	    //std::cerr << "JSCALE:" << jscale << std::endl;
	    if (!jscale.isNull()) {
		if (jscale.isArray()) {
		    scale = jscale[ind].asDouble();
		}
		else {
		    scale = jscale.asDouble();
		}
	    }

	    m_frame_scale.push_back(scale);
	    m_frame_tags.push_back(tag);
	}
	m_nticks = get(cfg, "nticks", m_nticks);
    }

    if (! cfg["summary_tags"].isNull()) {
	m_summary_scale.clear();
	auto jscale = cfg["summary_scale"];
	m_summary_tags.clear();
	auto jtags = cfg["summary_tags"];
	for (auto jtag : jtags) {
	    std::string tag = jtag.asString();

	    const int ind = m_summary_tags.size();
	    double scale = 1.0;
	    if (!jscale.isNull()) {
		if (jscale.isArray()) {
		    scale = jscale[ind].asDouble();
		}
		else {
		    scale = jscale.asDouble();
		}
	    }

	    m_summary_tags.push_back(tag);
	    m_summary_scale.push_back(scale);
	}
    }
}

void FrameSaver::produces(art::EDProducer* prod)
{
    assert(prod);

    for (auto tag : m_frame_tags) {
	if (!m_digitize) {
	    std::cerr << "wclsFrameSaver: promising to produce recob::Wires named \"" 
		      << tag << "\"\n";
	    prod->produces< std::vector<recob::Wire> >(tag);
	}
	else {
	    std::cerr << "wclsFrameSaver: promising to produce raw::RawDigits named \"" 
		      << tag << "\"\n";
	    prod->produces< std::vector<raw::RawDigit> >(tag);
	}
    }
    for (auto tag : m_summary_tags) {
	std::cerr << "wclsFrameSaver: promising to produce channel summary named \"" 
		  << tag << "\"\n";
	prod->produces< std::vector<double> >(tag);
    }
    for (auto cmm : m_cmms) {
	const std::string cmm_name = cmm.asString();
	std::cerr << "wclsFrameSaver: promising to produce channel masks named \"" 
		  << cmm_name << "\"\n";
	prod->produces< channel_list > (cmm_name + "channels");
	prod->produces< channel_masks > (cmm_name + "masks");
    }
}

// fixme: this is copied from sigproc/FrameUtil but it's currently private code.  
static
ITrace::vector tagged_traces(IFrame::pointer frame, IFrame::tag_t tag)
{
    ITrace::vector ret;
    auto const& all_traces = frame->traces();
    for (size_t index : frame->tagged_traces(tag)) {
        ret.push_back(all_traces->at(index));
    }
    if (!ret.empty()) {
        return ret;
    }
    auto ftags = frame->frame_tags();
    if (std::find(ftags.begin(), ftags.end(), tag) == ftags.end()) {
        return ret;
    }
    return *all_traces;		// must make copy of shared pointers
}

// Issolate some silly legacy shenanigans to keep the rest of the code
// blissfully ignorant of the evilness this implies.
struct PU {
    Json::Value pu;

    PU(Json::Value pu) : pu(pu) { }

    float operator()(int chid) {
	if (pu.isNumeric()) {
	    return pu.asFloat();
	}
	if (pu.asString() == "fiction") {
	    art::ServiceHandle<lariov::DetPedestalService> dps;
	    const auto& pv = dps->GetPedestalProvider();
	    return pv.PedMean(chid);
	}
	return 0.0;
    }
};
    
	

void FrameSaver::save_as_raw(art::Event & event)
{
    PU pu(m_pedestal_mean);

    const int ntags = m_frame_tags.size();
    for (int ind=0; ind<ntags; ++ind) {
	auto tag = m_frame_tags[ind];
        std::cerr << "wclsFrameSaver: saving raw::RawDigits tagged \"" << tag << "\"\n";

        if (!m_frame) { // is there someway to avoid this empty collection
                        // without annoying produces()?
            std::unique_ptr<std::vector<raw::RawDigit> > out(new std::vector<raw::RawDigit>);
            event.put(std::move(out), tag);
            continue;
        }

	auto traces = tagged_traces(m_frame, tag);
	if (traces.empty()) {
	    std::cerr << "wclsFrameSaver: no traces for tag \"" << tag << "\"\n";
	    continue;
	}

	//std::cerr << "wclsFrameSaver: got "<<traces.size()<<" traces for tag \"" << tag << "\"\n";
	std::unique_ptr<std::vector<raw::RawDigit> > out(new std::vector<raw::RawDigit>);

	double scale = m_frame_scale[ind];

	// what about the frame's time() and ident()?
	for (const auto& trace : traces) {
	    const int tbin = trace->tbin();
	    const int chid = trace->channel();
	    const auto& charge = trace->charge();

	    // enforce number of ticks if we are so configured.
	    size_t ncharge = charge.size();
	    int nticks = tbin + ncharge;
	    if (m_nticks) {	// force output waveform size
		if (m_nticks < nticks) {
		    ncharge  = m_nticks - tbin;
		}
		nticks = m_nticks;
	    }
	    raw::RawDigit::ADCvector_t adcv(nticks);
	    for (int ind=0; ind<nticks; ++ind) {
		adcv[ind] = scale * charge[ind]; // scale + truncate/redigitize
	    }
	    out->emplace_back(raw::RawDigit(chid, nticks, adcv, raw::kNone));
	    out->back().SetPedestal(pu(chid), m_pedestal_sigma);
	}
	event.put(std::move(out), tag);
    }
}


void FrameSaver::save_as_cooked(art::Event & event)
{
    const int ntags = m_frame_tags.size();
    for (int ind=0; ind<ntags; ++ind) {
	auto tag = m_frame_tags[ind];
        std::cerr << "wclsFrameSaver: saving recob::Wires tagged \"" << tag << "\"\n";

        if (!m_frame) { // is there someway to avoid this empty collection
                        // without annoying produces()?
            std::unique_ptr<std::vector<recob::Wire> > outwires(new std::vector<recob::Wire>);
            event.put(std::move(outwires), tag);
            continue;
        }
        

	auto traces = tagged_traces(m_frame, tag);
	if (traces.empty()) {
	    std::cerr << "wclsFrameSaver: no traces for tag \"" << tag << "\"\n";
	    continue;
	}

	double scale = m_frame_scale[ind];
	std::unique_ptr<std::vector<recob::Wire> > outwires(new std::vector<recob::Wire>);

	// what about the frame's time() and ident()?
	for (const auto& trace : traces) {
	    const int tbin = trace->tbin();
	    const int chid = trace->channel();
	    const auto& charge = trace->charge();

	    // enforce number of ticks if we are so configured.
	    size_t ncharge = charge.size();
	    int nticks = tbin + ncharge;
	    if (m_nticks) {	// force output waveform size
		if (m_nticks < nticks) {
		    ncharge  = m_nticks - tbin;
		}
		nticks = m_nticks;
	    }
	    recob::Wire::RegionsOfInterest_t roi(nticks);

	    if (m_sparse) {
		auto first = charge.begin();
		auto done = charge.end();
		auto beg = first;
		while (true) {
		    beg = std::find_if(beg, done, [](float v){return v != 0.0;});
		    if (beg == done) {
			break;
		    }
		    auto end = std::find_if(beg, done, [](float v){return v == 0.0;});

		    std::vector<float> scaled(beg, end);
		    for (int ind=0; ind<end-beg; ++ind) {
			scaled[ind] *= scale;
		    }
		    roi.add_range(tbin + beg-first, scaled.begin(), scaled.end());
		    beg = end;
		}
	    }
	    else {
		roi.add_range(tbin, charge.begin(), charge.begin() + ncharge);
	    }

	    // geo::kU, geo::kV, geo::kW
	    auto wpid = m_anode->resolve(chid);
	    geo::View_t view;
	    switch(wpid.layer()) {
	    case WireCell::kUlayer:
		view = geo::kU;
		break;
	    case WireCell::kVlayer:
		view = geo::kV;
		break;
	    case WireCell::kWlayer:
		view = geo::kW;
		break;
	    default:
		view = geo::kUnknown;
	    }
	    
	    // what about those pesky channel map masks?
	    // they are dropped for now.

	    outwires->emplace_back(recob::Wire(roi, chid, view));
	}
	event.put(std::move(outwires), tag);
    }
}

void FrameSaver::save_summaries(art::Event & event)
{
    const int ntags = m_summary_tags.size();
    //std::cerr << "wclsFrameSaver: " << ntags << " summary tags to save\n";
    for (int ind=0; ind<ntags; ++ind) {
	auto tag = m_summary_tags[ind];
        //std::cerr << "\t" << tag << "\n";

        std::unique_ptr<std::vector<double> > outsum(new std::vector<double>);

        if (m_frame) {
            const auto& summary = m_frame->trace_summary(tag);
            if (summary.empty()) {
                std::cerr << "wclsFrameSaver: no summary for tag \"" << tag << "\"\n";
                continue;
            }
            const double scale = m_summary_scale[ind];

            for (auto val : summary) {
                outsum->push_back(scale * val);
            }
        }

	event.put(std::move(outsum), tag);
    }
}

void FrameSaver::save_cmms(art::Event & event)
{
    if (m_cmms.isNull()) {
        return;
    }
    if (!m_cmms.isArray()) {
	std::cerr << "wclsFrameSaver: wrong type for configuration array of channel mask maps to save\n";
	return;
    }
    if (!m_frame) {
        return;
    }
    auto cmm = m_frame->masks();
    for (auto jcmm : m_cmms) {
	std::string name = jcmm.asString();

	auto it = cmm.find(name);
	if (it == cmm.end()) {
	    std::cerr << "wclsFrameSaver: failed to find requested channel masks \"" << name << "\"\n";
	    continue;
	}
	std::unique_ptr< channel_list > out_list(new channel_list);
	std::unique_ptr< channel_masks > out_masks(new channel_masks);
	for (auto cmit : it->second) { // int->vec<pair<int,int>>
	    out_list->push_back(cmit.first);
	    for (auto be : cmit.second) {
		out_masks->push_back(cmit.first);
		out_masks->push_back(be.first);
		out_masks->push_back(be.second);
	    }
	}
	if (out_list->empty()) {
	    std::cerr << "wclsFrameSaver: found empty channel masks for \"" << name << "\"\n";
	}
	event.put(std::move(out_list), name + "channels");
	event.put(std::move(out_masks), name + "masks");
    }
}

void FrameSaver::visit(art::Event & event)
{
    if (m_frame) {
        std::cerr << "wclsFrameSaver: saving frame to art::Event, frame has trace tags:[";
        for (auto tag : m_frame->trace_tags()) {
            std::cerr << " " << tag;
        }
        std::cerr << " ], and frame tags:[";
        for (auto tag : m_frame->frame_tags()) {
            std::cerr << " " << tag;
        }
        std::cerr << " ]\n";
    }
    else {
        std::cerr << "wclsFrameSaver: saving empty frame to art::Event\n";
    }

    if (m_digitize) {
	save_as_raw(event);
    }
    else {
	save_as_cooked(event);
    }

    save_summaries(event);

    save_cmms(event);

    m_frame = nullptr;		// done with stashed frame
}

bool FrameSaver::operator()(const WireCell::IFrame::pointer& inframe,
				  WireCell::IFrame::pointer& outframe)
{
    // set an IFrame based on last visited event.
    outframe = inframe;
    if (inframe) {
        if (m_frame) {
            std::cerr << "wclsFrameSaver: warning: dropping prior frame.  Fixme to handle queue of frames.\n";
        }
        // else {
        //     std::cerr << "wclsFrameSaver got frame\n";
        // }
        m_frame=inframe;
    }
    // else {
    //     std::cerr << "wclsFrameSaver sees EOS\n";
    // }
    return true;
}

// Local Variables:
// mode: c++
// c-basic-offset: 4
// End:
