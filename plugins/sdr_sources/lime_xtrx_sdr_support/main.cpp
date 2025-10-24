#include "core/plugin.h"
#include "logger.h"
#include "lime_xtrx_sdr.h"

class LimeXTRXSDRSupport : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "limesdr_sdr_support";
    }

    void init()
    {
        satdump::eventBus->register_handler<dsp::RegisterDSPSampleSourcesEvent>(registerSources);
    }

    static void registerSources(const dsp::RegisterDSPSampleSourcesEvent &evt)
    {
        evt.dsp_sources_registry.insert({LimeXTRXSDRSource::getID(), {LimeXTRXSDRSource::getInstance, LimeXTRXSDRSource::getAvailableSources}});
    }
};

PLUGIN_LOADER(LimeXTRXSDRSupport)