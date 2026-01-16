#include "airspyhf_sdr.h"
#include "core/plugin.h"
#include "dsp/flowgraph/dsp_flowgraph_register.h"
#include "logger.h"

#include "airspyhf_dev.h"

class AirspyHFSDRSupport : public satdump::Plugin
{
public:
    std::string getID() { return "airspyhf_sdr_support"; }

    void init()
    {
        satdump::eventBus->register_handler<dsp::RegisterDSPSampleSourcesEvent>(registerSources);

        satdump::eventBus->register_handler<satdump::ndsp::RequestDeviceListEvent>(registerDevs);
        satdump::eventBus->register_handler<satdump::ndsp::RequestDeviceInstanceEvent>(provideDeviceInstance);

        satdump::eventBus->register_handler<satdump::ndsp::RegisterNodesEvent>(registerNodes);
    }

    static void registerSources(const dsp::RegisterDSPSampleSourcesEvent &evt)
    {
        evt.dsp_sources_registry.insert({AirspyHFSource::getID(), {AirspyHFSource::getInstance, AirspyHFSource::getAvailableSources}});
    }

    static void registerDevs(const satdump::ndsp::RequestDeviceListEvent &evt)
    {
        if (evt.m == satdump::ndsp::DeviceBlock::MODE_SINGLE_TX)
            return;

        auto d = satdump::ndsp::AirspyHFDevBlock::listDevs();
        evt.i.insert(evt.i.end(), d.begin(), d.end());
    }

    static void provideDeviceInstance(const satdump::ndsp::RequestDeviceInstanceEvent &evt)
    {
        if (evt.info.type == "airspyhf")
            evt.i.push_back(std::make_shared<satdump::ndsp::AirspyHFDevBlock>());
    }

    static void registerNodes(const satdump::ndsp::RegisterNodesEvent &evt)
    {
        evt.r.insert(
            {"airspyhf_cc",
             {"Device/AirspyHF Dev", [](const satdump::ndsp::Flowgraph *f) { return std::make_shared<satdump::ndsp::NodeInternal>(f, std::make_shared<satdump::ndsp::AirspyHFDevBlock>()); }}});
    }
};

PLUGIN_LOADER(AirspyHFSDRSupport)