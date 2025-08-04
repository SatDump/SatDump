#include "core/plugin.h"
#include "logger.h"
#include "harogic_sdr.h"

class HarogicSDRSupport : public satdump::Plugin
{
public:
    std::string getID() override
    {
        return "harogic_sdr_support";
    }

    void init() override
    {
        // No global API init needed for HTRA
        logger->info("Loaded Harogic SDR Support Plugin.");
        satdump::eventBus->register_handler<dsp::RegisterDSPSampleSourcesEvent>(registerSources);
    }

    static void registerSources(const dsp::RegisterDSPSampleSourcesEvent &evt)
    {
        evt.dsp_sources_registry.insert({HarogicSource::getID(), {HarogicSource::getInstance, HarogicSource::getAvailableSources}});
    }
};

PLUGIN_LOADER(HarogicSDRSupport)
