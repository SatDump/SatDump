#include "core/plugin.h"
#include "dsp/flowgraph/dsp_flowgraph_register.h"
#include "lime_sdr.h"
#include "limesdr_dev.h"
#include "logger.h"

class LimeSDRSupport : public satdump::Plugin
{
public:
    std::string getID() { return "limesdr_sdr_support"; }

    void init()
    {
        satdump::eventBus->register_handler<dsp::RegisterDSPSampleSourcesEvent>(registerSources);

        satdump::eventBus->register_handler<satdump::ndsp::RequestDeviceListEvent>(registerDevs);
        satdump::eventBus->register_handler<satdump::ndsp::RequestDeviceInstanceEvent>(provideDeviceInstance);

        satdump::eventBus->register_handler<satdump::ndsp::flowgraph::RegisterNodesEvent>(registerNodes);
    }

    static void registerSources(const dsp::RegisterDSPSampleSourcesEvent &evt)
    {
        evt.dsp_sources_registry.insert({LimeSDRSource::getID(), {LimeSDRSource::getInstance, LimeSDRSource::getAvailableSources}});
    }

    static void registerDevs(const satdump::ndsp::RequestDeviceListEvent &evt)
    {
        if (evt.m != satdump::ndsp::DeviceBlock::MODE_SINGLE_RX && //
            evt.m != satdump::ndsp::DeviceBlock::MODE_SINGLE_TX && //
            evt.m != satdump::ndsp::DeviceBlock::MODE_RX_TX &&     //
            evt.m != satdump::ndsp::DeviceBlock::MODE_NORMAL)
            return;

        auto d = satdump::ndsp::LimeSDRDevBlock::listDevs();
        evt.i.insert(evt.i.end(), d.begin(), d.end());
    }

    static void provideDeviceInstance(const satdump::ndsp::RequestDeviceInstanceEvent &evt)
    {
        if (evt.info.type == "limesdr")
            evt.i.push_back(std::make_shared<satdump::ndsp::LimeSDRDevBlock>());
    }

    static void registerNodes(const satdump::ndsp::flowgraph::RegisterNodesEvent &evt)
    {
        evt.r.insert({"limesdr_cc",
                      {"Device/LimeSDR Dev",
                       [](const satdump::ndsp::flowgraph::Flowgraph *f) { return std::make_shared<satdump::ndsp::flowgraph::NodeInternal>(f, std::make_shared<satdump::ndsp::LimeSDRDevBlock>()); }}});
    }
};

PLUGIN_LOADER(LimeSDRSupport)