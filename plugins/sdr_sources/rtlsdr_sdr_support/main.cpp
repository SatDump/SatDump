#include "core/plugin.h"
#include "dsp/device/dev.h"
#include "logger.h"
#include "rtlsdr_sdr.h"

#include "dsp/flowgraph/dsp_flowgraph_register.h"
#include "rtlsdr_dev.h"

class RtlSdrSDRSupport : public satdump::Plugin
{
public:
    std::string getID() { return "rtlsdr_sdr_support"; }

    void init()
    {
        satdump::eventBus->register_handler<dsp::RegisterDSPSampleSourcesEvent>(registerSources);

        satdump::eventBus->register_handler<satdump::ndsp::RequestDeviceListEvent>(registerDevs);
        satdump::eventBus->register_handler<satdump::ndsp::RequestDeviceInstanceEvent>(provideDeviceInstance);

        satdump::eventBus->register_handler<satdump::ndsp::RegisterNodesEvent>(registerNodes);
    }

    static void registerSources(const dsp::RegisterDSPSampleSourcesEvent &evt)
    {
        evt.dsp_sources_registry.insert({RtlSdrSource::getID(), {RtlSdrSource::getInstance, RtlSdrSource::getAvailableSources}});
    }

    static void registerDevs(const satdump::ndsp::RequestDeviceListEvent &evt)
    {
        if (evt.m != satdump::ndsp::DeviceBlock::MODE_SINGLE_RX && evt.m != satdump::ndsp::DeviceBlock::MODE_NORMAL)
            return;

        auto d = satdump::ndsp::RTLSDRDevBlock::listDevs();
        evt.i.insert(evt.i.end(), d.begin(), d.end());
    }

    static void provideDeviceInstance(const satdump::ndsp::RequestDeviceInstanceEvent &evt)
    {
        if (evt.info.type == "rtlsdr")
            evt.i.push_back(std::make_shared<satdump::ndsp::RTLSDRDevBlock>());
    }

    static void registerNodes(const satdump::ndsp::RegisterNodesEvent &evt)
    {
        evt.r.insert({"rtlsdr_cc",
                      {"Device/RTL-SDR Dev", [](const satdump::ndsp::Flowgraph *f) { return std::make_shared<satdump::ndsp::NodeInternal>(f, std::make_shared<satdump::ndsp::RTLSDRDevBlock>()); }}});
    }
};

PLUGIN_LOADER(RtlSdrSDRSupport)