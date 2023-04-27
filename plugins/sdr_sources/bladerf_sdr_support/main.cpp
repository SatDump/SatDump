#include "core/plugin.h"
#include "logger.h"
#include "bladerf_sdr_source.h"
#include "bladerf_sdr_sink.h"

class BladeRFSDRSupport : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "bladerf_sdr_support";
    }

    void init()
    {
        satdump::eventBus->register_handler<dsp::RegisterDSPSampleSourcesEvent>(registerSources);
        satdump::eventBus->register_handler<dsp::RegisterDSPSampleSinksEvent>(registerSinks);
    }

    static void registerSources(const dsp::RegisterDSPSampleSourcesEvent &evt)
    {
        evt.dsp_sources_registry.insert({BladeRFSource::getID(), {BladeRFSource::getInstance, BladeRFSource::getAvailableSources}});
    }

    static void registerSinks(const dsp::RegisterDSPSampleSinksEvent &evt)
    {
        evt.dsp_sinks_registry.insert({BladeRFSink::getID(), {BladeRFSink::getInstance, BladeRFSink::getAvailableSinks}});
    }
};

PLUGIN_LOADER(BladeRFSDRSupport)