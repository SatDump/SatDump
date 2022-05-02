#include "core/plugin.h"
#include "logger.h"
#include "hackrf_sdr.h"

class HackRFSDRSupport : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "hackrf_sdr_support";
    }

    void init()
    {
        satdump::eventBus->register_handler<dsp::RegisterDSPSampleSourcesEvent>(registerSources);
        hackrf_init();
    }

    static void registerSources(const dsp::RegisterDSPSampleSourcesEvent &evt)
    {
        evt.dsp_sources_registry.insert({HackRFSource::getID(), {HackRFSource::getInstance, HackRFSource::getAvailableSources}});
    }
};

PLUGIN_LOADER(HackRFSDRSupport)