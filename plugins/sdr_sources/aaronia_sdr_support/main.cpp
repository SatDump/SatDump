#include "core/plugin.h"
#include "logger.h"
#include "aaronia_sdr.h"

class AaroniaSDRSupport : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "aaronia_sdr_support";
    }

    void init()
    {
        if (AARTSAAPI_Init(AARTSAAPI_MEMORY_SMALL) != AARTSAAPI_OK)
            logger->error("Could not init Aaronia API!");
        else
            logger->info("Loaded Aaronia API!");

        satdump::eventBus->register_handler<dsp::RegisterDSPSampleSourcesEvent>(registerSources);
    }

    static void registerSources(const dsp::RegisterDSPSampleSourcesEvent &evt)
    {
        evt.dsp_sources_registry.insert({AaroniaSource::getID(), {AaroniaSource::getInstance, AaroniaSource::getAvailableSources}});
    }
};

PLUGIN_LOADER(AaroniaSDRSupport)