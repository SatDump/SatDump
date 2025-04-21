#include "bladerf_dev.h"
#include "bladerf_sdr_sink.h"
#include "bladerf_sdr_source.h"
#include "core/plugin.h"
#include "dsp/flowgraph/dsp_flowgraph_handler.h"
#include "dsp/flowgraph/flowgraph.h"
#include "logger.h"

class BladeRFSDRSupport : public satdump::Plugin
{
public:
    std::string getID() { return "bladerf_sdr_support"; }

    void init()
    {
        satdump::eventBus->register_handler<dsp::RegisterDSPSampleSourcesEvent>(registerSources);
        satdump::eventBus->register_handler<dsp::RegisterDSPSampleSinksEvent>(registerSinks);

        satdump::eventBus->register_handler<satdump::ndsp::RequestDeviceListEvent>(registerDevs);
        satdump::eventBus->register_handler<satdump::ndsp::RequestDeviceInstanceEvent>(provideDeviceInstance);

        satdump::eventBus->register_handler<satdump::viewer::RegisterNodesEvent>(registerNodes);
    }

    static void registerSources(const dsp::RegisterDSPSampleSourcesEvent &evt)
    {
        evt.dsp_sources_registry.insert({BladeRFSource::getID(), {BladeRFSource::getInstance, BladeRFSource::getAvailableSources}});
    }

    static void registerSinks(const dsp::RegisterDSPSampleSinksEvent &evt) { evt.dsp_sinks_registry.insert({BladeRFSink::getID(), {BladeRFSink::getInstance, BladeRFSink::getAvailableSinks}}); }

    static void registerDevs(const satdump::ndsp::RequestDeviceListEvent &evt)
    {
        auto d = satdump::ndsp::BladeRFDevBlock::listDevs();
        evt.i.insert(evt.i.end(), d.begin(), d.end());
    }

    static void provideDeviceInstance(const satdump::ndsp::RequestDeviceInstanceEvent &evt)
    {
        if (evt.info.type == "bladerf")
            evt.i.push_back(std::make_shared<satdump::ndsp::BladeRFDevBlock>());
    }

    static void registerNodes(const satdump::viewer::RegisterNodesEvent &evt)
    {
        evt.r.insert({"bladerf_cc",
                      {"Device/BladeRF Dev", [](const satdump::ndsp::Flowgraph *f) { return std::make_shared<satdump::ndsp::NodeInternal>(f, std::make_shared<satdump::ndsp::BladeRFDevBlock>()); }}});
    }
};

PLUGIN_LOADER(BladeRFSDRSupport)