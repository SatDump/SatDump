#include "core/plugin.h"
#include "logger.h"
#include "bladerf_sdr.h"

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
    }

    static void registerSources(const dsp::RegisterDSPSampleSourcesEvent &evt)
    {
        evt.dsp_sources_registry.insert({BladeRFSource::getID(), {BladeRFSource::getInstance, BladeRFSource::getAvailableSources}});
    }
};

PLUGIN_LOADER(BladeRFSDRSupport)