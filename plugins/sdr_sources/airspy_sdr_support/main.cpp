#include "airspy_sdr.h"
#include "core/plugin.h"
#include "dsp/flowgraph/dsp_flowgraph_handler.h"
#include "logger.h"

#include "airspy_dev.h"

class AirspySDRSupport : public satdump::Plugin
{
public:
    std::string getID() { return "airspy_sdr_support"; }

    void init()
    {
        satdump::eventBus->register_handler<dsp::RegisterDSPSampleSourcesEvent>(registerSources);

        satdump::eventBus->register_handler<satdump::ndsp::RequestDeviceListEvent>(registerDevs);
        satdump::eventBus->register_handler<satdump::ndsp::RequestDeviceInstanceEvent>(provideDeviceInstance);

        satdump::eventBus->register_handler<satdump::handlers::RegisterNodesEvent>(registerNodes);
    }

    static void registerSources(const dsp::RegisterDSPSampleSourcesEvent &evt)
    {
        evt.dsp_sources_registry.insert({AirspySource::getID(), {AirspySource::getInstance, AirspySource::getAvailableSources}});
    }

    static void registerDevs(const satdump::ndsp::RequestDeviceListEvent &evt)
    {
        if (evt.m == satdump::ndsp::DeviceBlock::MODE_SINGLE_TX)
            return;

        auto d = satdump::ndsp::AirspyDevBlock::listDevs();
        evt.i.insert(evt.i.end(), d.begin(), d.end());
    }

    static void provideDeviceInstance(const satdump::ndsp::RequestDeviceInstanceEvent &evt)
    {
        if (evt.info.type == "airspy")
            evt.i.push_back(std::make_shared<satdump::ndsp::AirspyDevBlock>());
    }

    static void registerNodes(const satdump::handlers::RegisterNodesEvent &evt)
    {
        evt.r.insert({"airspy_cc",
                      {"Device/Airspy Dev", [](const satdump::ndsp::Flowgraph *f) { return std::make_shared<satdump::ndsp::NodeInternal>(f, std::make_shared<satdump::ndsp::AirspyDevBlock>()); }}});
    }
};

PLUGIN_LOADER(AirspySDRSupport)