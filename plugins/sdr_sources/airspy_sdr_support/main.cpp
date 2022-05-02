#include "core/plugin.h"
#include "logger.h"
#include "airspy_sdr.h"

class AirspySDRSupport : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "airspy_sdr_support";
    }

    void init()
    {
        satdump::eventBus->register_handler<dsp::RegisterDSPSampleSourcesEvent>(registerSources);
    }

    static void registerSources(const dsp::RegisterDSPSampleSourcesEvent &evt)
    {
        evt.dsp_sources_registry.insert({AirspySource::getID(), {AirspySource::getInstance, AirspySource::getAvailableSources}});
    }
};

PLUGIN_LOADER(AirspySDRSupport)