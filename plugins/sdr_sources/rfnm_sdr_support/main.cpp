#include "core/plugin.h"
#include "logger.h"
#include "rfnm_sdr.h"

class RfNmSDRSupport : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "rfnm_sdr_support";
    }

    void init()
    {
        satdump::eventBus->register_handler<dsp::RegisterDSPSampleSourcesEvent>(registerSources);
    }

    static void registerSources(const dsp::RegisterDSPSampleSourcesEvent &evt)
    {
        evt.dsp_sources_registry.insert({RFNMSource::getID(), {RFNMSource::getInstance, RFNMSource::getAvailableSources}});
    }
};

PLUGIN_LOADER(RfNmSDRSupport)