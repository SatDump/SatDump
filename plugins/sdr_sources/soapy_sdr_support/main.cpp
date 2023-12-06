#include "core/plugin.h"
#include "logger.h"
#include "soapysdr_sdr.h"

class SoapySDRSupport : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "soapy_sdr_support";
    }

    void init()
    {
        satdump::eventBus->register_handler<dsp::RegisterDSPSampleSourcesEvent>(registerSources);
    }

    static void registerSources(const dsp::RegisterDSPSampleSourcesEvent &evt)
    {
        evt.dsp_sources_registry.insert({SoapySdrSource::getID(), {SoapySdrSource::getInstance, SoapySdrSource::getAvailableSources}});
    }
};

PLUGIN_LOADER(SoapySDRSupport)