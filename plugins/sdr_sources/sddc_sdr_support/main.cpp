#include "core/plugin.h"
#include "logger.h"
#include "sddc_sdr.h"

class SddcSDRSupport : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "sddc_sdr_support";
    }

    void init()
    {
        satdump::eventBus->register_handler<dsp::RegisterDSPSampleSourcesEvent>(registerSources);
    }

    static void registerSources(const dsp::RegisterDSPSampleSourcesEvent &evt)
    {
        evt.dsp_sources_registry.insert({SDDCSource::getID(), {SDDCSource::getInstance, SDDCSource::getAvailableSources}});
    }
};

PLUGIN_LOADER(SddcSDRSupport)