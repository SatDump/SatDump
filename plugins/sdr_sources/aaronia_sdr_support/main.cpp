#include "aaronia_sdr.h"
#include "core/config.h"
#include "core/plugin.h"
#include "dynload.h"

class AaroniaSDRSupport : public satdump::Plugin
{
private:
    static void renderConfig() { ImGui::InputText("RTSA API Path", &rtsa_api_path); }

    static void save() { satdump::satdump_cfg.main_cfg["plugin_settings"]["aaronia_support"] = rtsa_api_path; }

public:
    std::string getID() { return "aaronia_sdr_support"; }

    void init()
    {
        satdump::eventBus->register_handler<satdump::config::RegisterPluginConfigHandlersEvent>(registerConfigHandler);
        if (!satdump::satdump_cfg.main_cfg["plugin_settings"]["aaronia_support"].is_null())
            rtsa_api_path = satdump::satdump_cfg.main_cfg["plugin_settings"]["aaronia_support"];

        rtsa_api = new RTSAApiInstance(rtsa_api_path);

        if (rtsa_api->isOpen())
            satdump::eventBus->register_handler<dsp::RegisterDSPSampleSourcesEvent>(registerSources);
    }

    static void registerSources(const dsp::RegisterDSPSampleSourcesEvent &evt)
    {
        evt.dsp_sources_registry.insert({AaroniaSource::getID(), {AaroniaSource::getInstance, AaroniaSource::getAvailableSources}});
    }

    static void registerConfigHandler(const satdump::config::RegisterPluginConfigHandlersEvent &evt)
    {
        evt.plugin_config_handlers.push_back({"Aaronia Support", AaroniaSDRSupport::renderConfig, AaroniaSDRSupport::save});
    }

    // ~AaroniaSDRSupport() { delete rtsa_api; }
};

#ifdef RTSA_LIB_PATH
std::string rtsa_api_path = RTSA_LIB_PATH;
#else
std::string rtsa_api_path = "/opt/aaronia-rtsa-suite/Aaronia-RTSA-Suite-PRO/libAaroniaRTSAAPI.so";
#endif

RTSAApiInstance *rtsa_api = nullptr;

PLUGIN_LOADER(AaroniaSDRSupport)