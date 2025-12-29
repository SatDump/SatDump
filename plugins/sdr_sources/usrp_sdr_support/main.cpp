#include "core/config.h"
#include "core/params.h"
#include "core/plugin.h"
#include "init.h"
#include "logger.h"
#include "usrp_sdr.h"

#ifdef _WIN32
errno_t setenv(const char* name, const char* value, int)
{
    return _putenv_s(name, value);
}
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
        setenv("UHD_IMAGES_DIR", file_select->getPath().c_str(), 1);
        if (file_select->getPath() != "")
            satdump::db->set_user("usrp_sdr_support/uhd_images_dir", file_select->getPath());
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
        std::string val = satdump::db->get_user("usrp_sdr_support/uhd_images_dir", "");
        if (val != "")
        {
            file_select->setPath(val);
            setenv("UHD_IMAGES_DIR", file_select->getPath().c_str(), 1);
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