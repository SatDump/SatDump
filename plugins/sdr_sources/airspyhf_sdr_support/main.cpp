#include "core/plugin.h"
#include "logger.h"
#include "airspyhf_sdr.h"

class AirspyHFSDRSupport : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "airspyhf_sdr_support";
    }

    void init()
    {
        satdump::eventBus->register_handler<dsp::RegisterDSPSampleSourcesEvent>(registerSources);
    }

    static void registerSources(const dsp::RegisterDSPSampleSourcesEvent &evt)
    {
        evt.dsp_sources_registry.insert({AirspyHFSource::getID(), {AirspyHFSource::getInstance, AirspyHFSource::getAvailableSources}});
    }
};

PLUGIN_LOADER(AirspyHFSDRSupport)