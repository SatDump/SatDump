#include "core/config.h"
#include "core/params.h"
#include "core/plugin.h"
#include "logger.h"
#include "usrp_sdr.h"

#ifdef _WIN32
#define setenv _putenv_s
#else
#include <stdlib.h>
#endif

std::shared_ptr<FileSelectWidget> file_select;

class USRPSDRSupport : public satdump::Plugin
{
public:
    static void renderConfig()
    {
        static bool use_default = (file_select->getPath() == "");
        if (ImGui::BeginTable("##usrpsdrsettings", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
        {
            // Theme Selection
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted("Use default UHD firmware images path");
            ImGui::TableSetColumnIndex(1);
            if (ImGui::Checkbox("##usrpimagespathcheckbox", &use_default) && use_default)
                file_select->setPath("");
            if (!use_default)
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted("UHD Images Path");
                ImGui::TableSetColumnIndex(1);
                file_select->draw();
            }
            ImGui::EndTable();
        }
    }

    static void save()
    {
        setenv("UHD_IMAGES_DIR", file_select->getPath().c_str());
        satdump::config::main_cfg["plugin_settings"]["usrp_sdr_support"] = nlohmann::json::object();
        if (file_select->getPath() != "")
            satdump::config::main_cfg["plugin_settings"]["usrp_sdr_support"]["uhd_images_dir"] = file_select->getPath();
    }

    std::string getID()
    {
        return "usrp_sdr_support";
    }

    void init()
    {
        satdump::eventBus->register_handler<dsp::RegisterDSPSampleSourcesEvent>(registerSources);
        satdump::eventBus->register_handler<satdump::config::RegisterPluginConfigHandlersEvent>(registerConfigHandler);

        file_select = std::make_shared<FileSelectWidget>("UHD Images Path", "UHD Images Path", true);
        if (satdump::config::main_cfg["plugin_settings"].contains("usrp_sdr_support") &&
            satdump::config::main_cfg["plugin_settings"]["usrp_sdr_support"].contains("uhd_images_dir") &&
            satdump::config::main_cfg["plugin_settings"]["usrp_sdr_support"]["uhd_images_dir"].is_string() &&
            satdump::config::main_cfg["plugin_settings"]["usrp_sdr_support"]["uhd_images_dir"] != "")
        {
            file_select->setPath(satdump::config::main_cfg["plugin_settings"]["usrp_sdr_support"]["uhd_images_dir"]);
            setenv("UHD_IMAGES_DIR", file_select->getPath().c_str());
        }
    }

    static void registerSources(const dsp::RegisterDSPSampleSourcesEvent &evt)
    {
        evt.dsp_sources_registry.insert({USRPSource::getID(), {USRPSource::getInstance, USRPSource::getAvailableSources}});
    }

    static void registerConfigHandler(const satdump::config::RegisterPluginConfigHandlersEvent &evt)
    {
        evt.plugin_config_handlers.push_back({ "USRP SDR", USRPSDRSupport::renderConfig, USRPSDRSupport::save });
    }
};

PLUGIN_LOADER(USRPSDRSupport)