#include "core/plugin.h"
#include "logger.h"
#include "sdrplay_sdr.h"

class SDRPlaySDRSupport : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "sdrplay_sdr_support";
    }

    void init()
    {
        satdump::eventBus->register_handler<dsp::RegisterDSPSampleSourcesEvent>(registerSources);

        sdrplay_api_ErrT error = sdrplay_api_Open();
        if (error != sdrplay_api_Success)
        {
            logger->error("Could not open the SDRPlay API, perhaps the service is not running?");
            return;
        }
        logger->info("SDRPlay API is ready!");
    }

    static void registerSources(const dsp::RegisterDSPSampleSourcesEvent &evt)
    {
        evt.dsp_sources_registry.insert({SDRPlaySource::getID(), {SDRPlaySource::getInstance, SDRPlaySource::getAvailableSources}});
    }
};

PLUGIN_LOADER(SDRPlaySDRSupport)