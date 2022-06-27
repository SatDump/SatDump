#include "core/plugin.h"
#include "logger.h"
#include "plutosdr_sdr.h"

class PlutoSDRSupport : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "plutosdr_sdr_support";
    }

    void init()
    {
        satdump::eventBus->register_handler<dsp::RegisterDSPSampleSourcesEvent>(registerSources);
    }

    static void registerSources(const dsp::RegisterDSPSampleSourcesEvent &evt)
    {
        evt.dsp_sources_registry.insert({PlutoSDRSource::getID(), {PlutoSDRSource::getInstance, PlutoSDRSource::getAvailableSources}});
    }
};

PLUGIN_LOADER(PlutoSDRSupport)