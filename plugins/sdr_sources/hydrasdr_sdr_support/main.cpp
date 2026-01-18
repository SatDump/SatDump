#include "core/plugin.h"
#include "dsp/device/dev.h"
#include "dsp/flowgraph/dsp_flowgraph_register.h"
#include "hydrasdr_dev.h"
#include "hydrasdr_sdr.h"

class HydraDRSupport : public satdump::Plugin
{
public:
    std::string getID() { return "hydrasdr_sdr_support"; }

    void init()
    {
        satdump::eventBus->register_handler<dsp::RegisterDSPSampleSourcesEvent>(registerSources);

        satdump::eventBus->register_handler<satdump::ndsp::RequestDeviceListEvent>(registerDevs);
        satdump::eventBus->register_handler<satdump::ndsp::RequestDeviceInstanceEvent>(provideDeviceInstance);

        satdump::eventBus->register_handler<satdump::ndsp::RegisterNodesEvent>(registerNodes);
    }

    static void registerSources(const dsp::RegisterDSPSampleSourcesEvent &evt)
    {
        evt.dsp_sources_registry.insert({HydraSDRSource::getID(), {HydraSDRSource::getInstance, HydraSDRSource::getAvailableSources}});
    }

    static void registerDevs(const satdump::ndsp::RequestDeviceListEvent &evt)
    {
        if (evt.m == satdump::ndsp::DeviceBlock::MODE_SINGLE_TX)
            return;

        auto d = satdump::ndsp::HydraSDRDevBlock::listDevs();
        evt.i.insert(evt.i.end(), d.begin(), d.end());
    }

    static void provideDeviceInstance(const satdump::ndsp::RequestDeviceInstanceEvent &evt)
    {
        if (evt.info.type == "hydrasdr")
            evt.i.push_back(std::make_shared<satdump::ndsp::HydraSDRDevBlock>());
    }

    static void registerNodes(const satdump::ndsp::RegisterNodesEvent &evt)
    {
        evt.r.insert(
            {"hydrasdr_cc",
             {"Device/HydraSDR Dev", [](const satdump::ndsp::Flowgraph *f) { return std::make_shared<satdump::ndsp::NodeInternal>(f, std::make_shared<satdump::ndsp::HydraSDRDevBlock>()); }}});
    }
};

PLUGIN_LOADER(HydraDRSupport)