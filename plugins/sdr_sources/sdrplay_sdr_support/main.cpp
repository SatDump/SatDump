#include "core/config.h"
#include "core/plugin.h"
#include "imgui/imgui.h"
#include "imgui/imgui_stdlib.h"
#include "logger.h"
#include "sdrplay_sdr.h"

class SDRPlaySDRSupport : public satdump::Plugin
{
private:
    static void renderConfig() { ImGui::InputText("SDRPlay API Path", &sdrplay_api_path); }

    static void save() { satdump::satdump_cfg.main_cfg["plugin_settings"]["sdrplay_support"] = sdrplay_api_path; }

public:
    std::string getID() { return "sdrplay_sdr_support"; }

    void init()
    {
        satdump::eventBus->register_handler<dsp::RegisterDSPSampleSourcesEvent>(registerSources);
        satdump::eventBus->register_handler<satdump::config::RegisterPluginConfigHandlersEvent>(registerConfigHandler);
        if (!satdump::satdump_cfg.main_cfg["plugin_settings"]["sdrplay_support"].is_null())
            sdrplay_api_path = satdump::satdump_cfg.main_cfg["plugin_settings"]["sdrplay_support"];
    }

    static void registerSources(const dsp::RegisterDSPSampleSourcesEvent &evt)
    {
        evt.dsp_sources_registry.insert({SDRPlaySource::getID(), {SDRPlaySource::getInstance, SDRPlaySource::getAvailableSources}});
    }

    static void registerConfigHandler(const satdump::config::RegisterPluginConfigHandlersEvent &evt)
    {
        evt.plugin_config_handlers.push_back({"SDRPlay Support", SDRPlaySDRSupport::renderConfig, SDRPlaySDRSupport::save});
    }
};

#ifdef SDRPLAY_LIB_PATH
std::string sdrplay_api_path = SDRPLAY_LIB_PATH;
#else
std::string sdrplay_api_path = "libsdrplay.so";
#endif

PLUGIN_LOADER(SDRPlaySDRSupport)