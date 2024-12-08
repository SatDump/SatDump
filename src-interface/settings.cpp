#include "settings.h"
#include "imgui/imgui.h"
#include <string>
#include "core/params.h"

#include "core/config.h"

#include "main_ui.h"
#include "core/opencl.h"

#include "init.h"
#include "common/tracking/tle.h"
#include "common/widgets/timed_message.h"
#include "common/widgets/json_editor.h"

#include "core/style.h"

namespace satdump
{
    namespace settings
    {
        std::vector<std::pair<std::string, satdump::params::EditableParameter>> settings_user_interface;
        std::vector<std::pair<std::string, satdump::params::EditableParameter>> settings_general;
        std::vector<std::pair<std::string, satdump::params::EditableParameter>> settings_output_directories;

#ifdef USE_OPENCL
        // OpenCL Selection
        int opencl_devices_id = 0;
        std::string opencl_devices_str;
        std::vector<opencl::OCLDevice> opencl_devices_enum;
#endif

        int selected_theme = 0;
        std::vector<std::string> themes;
        std::string themes_str = "";

        bool tles_are_update = false;
        char tle_last_update[80];

        bool show_imgui_demo = false;
        bool advanced_mode = false;

        widgets::TimedMessage saved_message;

        void setup()
        {
            nlohmann::ordered_json params = satdump::config::main_cfg["user_interface"];

            for (nlohmann::detail::iteration_proxy_value<nlohmann::detail::iter_impl<nlohmann::ordered_json>> cfg : params.items())
            {
                // Check setting type, and create an EditableParameter if possible
                if (cfg.value().contains("type") && cfg.value().contains("value") && cfg.value().contains("name"))
                    settings_user_interface.push_back({cfg.key(), params::EditableParameter(nlohmann::json(cfg.value()))});
            }

            params = satdump::config::main_cfg["satdump_general"];

            for (nlohmann::detail::iteration_proxy_value<nlohmann::detail::iter_impl<nlohmann::ordered_json>> cfg : params.items())
            {
                // Check setting type, and create an EditableParameter if possible
                if (cfg.value().contains("type") && cfg.value().contains("value") && cfg.value().contains("name"))
                    settings_general.push_back({cfg.key(), params::EditableParameter(nlohmann::json(cfg.value()))});
            }

            params = satdump::config::main_cfg["satdump_directories"];

            for (nlohmann::detail::iteration_proxy_value<nlohmann::detail::iter_impl<nlohmann::ordered_json>> cfg : params.items())
            {
                // Check setting type, and create an EditableParameter if possible
                if (cfg.value().contains("type") && cfg.value().contains("value") && cfg.value().contains("name"))
                    settings_output_directories.push_back({cfg.key(), params::EditableParameter(nlohmann::json(cfg.value()))});
            }

            int theme_id = 0;
            std::string current_theme = satdump::config::main_cfg["user_interface"]["theme"]["value"].get<std::string>();
            for (const auto& entry : std::filesystem::directory_iterator(resources::getResourcePath("themes")))
            {
                if (entry.path().filename().extension() != ".json")
                    continue;
                std::string this_name = entry.path().filename().stem().string();
                themes.push_back(this_name);
                themes_str += this_name;
                themes_str.push_back('\0');
                if (this_name == current_theme)
                    selected_theme = theme_id;
                theme_id++;
            }

            advanced_mode = getValueOrDefault(satdump::config::main_cfg["user_interface"]["advanced_mode"]["value"], false);

#ifdef USE_OPENCL
            opencl_devices_enum = opencl::getAllDevices();
            opencl_devices_enum.push_back({ -1, -1, "None (Use CPU)" });
            int p = satdump::config::main_cfg["satdump_general"]["opencl_device"]["platform"].get<int>();
            int d = satdump::config::main_cfg["satdump_general"]["opencl_device"]["device"].get<int>();
            int dev_id = 0;
            opencl_devices_str = "";
            for (opencl::OCLDevice &dev : opencl_devices_enum)
            {
                opencl_devices_str += dev.name;
                if (dev.platform_id == p && dev.device_id == d)
                    opencl_devices_id = dev_id;
                dev_id++;
            }
            opencl_devices_str.push_back('\0');
#endif
        }

        void render()
        {
            ImGui::SeparatorText("Core Settings");
            if (ImGui::CollapsingHeader("User Interface"))
            {
                if (ImGui::BeginTable("##satdumpuisettings", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
                {
                    // Theme Selection
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Theme");
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Set the style and color of SatDump");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Combo("##themeselection", &selected_theme, themes_str.c_str());

                    // Standard user interface settings
                    for (std::pair<std::string, satdump::params::EditableParameter> &p : settings_user_interface)
                        p.second.draw();

                    // ImGui Demo
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Show ImGui Demo");
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("For developers only!");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Checkbox("##showimguidebugcheckbox", &show_imgui_demo);

                    ImGui::EndTable();
                }
            }

            if (ImGui::CollapsingHeader("General SatDump"))
            {
                if (ImGui::BeginTable("##satdumpgeneralsettings", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
                {
#ifdef USE_OPENCL
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("OpenCL Device");
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("OpenCL Device SatDump will use for accelerated computing where it can help, eg, for some image processing tasks such as projections.");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Combo("##opencldeviceselection", &opencl_devices_id, opencl_devices_str.c_str());
#endif

                    for (std::pair<std::string, satdump::params::EditableParameter> &p : settings_general)
                        p.second.draw();

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Update TLEs Now");
                    ImGui::TableSetColumnIndex(1);
                    bool disable_update_button = tles_are_update;
                    if (disable_update_button)
                        style::beginDisabled();
                    if (ImGui::Button("Update###updateTLEs"))
                    {
                        ui_thread_pool.push([](int)
                                            {   tles_are_update = true;
                                                updateTLEFile(satdump::user_path + "/satdump_tles.txt"); 
                                                tles_are_update = false; });
                    }
                    if (disable_update_button)
                        style::endDisabled();

                    time_t last_update = getValueOrDefault<time_t>(config::main_cfg["user"]["tles_last_updated"], 0);
                    if (last_update == 0)
                        strcpy(tle_last_update, "Never");
                    else
                    {
                        struct tm ts;
                        ts = *gmtime(&last_update);
                        strftime(tle_last_update, sizeof(tle_last_update), "%Y-%m-%d %H:%M:%S UTC", &ts);
                    }
                    ImGui::SameLine(0.0f, 10.0f * ui_scale);
                    ImGui::TextDisabled("Last updated: %s", tle_last_update);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Clear Tile Map (OSM) Cache");
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Delete all cached tiles (OSM, and other sources).");
                    ImGui::TableSetColumnIndex(1);
                    if (ImGui::Button("Clear Cache###deleteosmtiles"))
                        if (std::filesystem::exists(satdump::user_path + "/osm_tiles/"))
                            std::filesystem::remove_all(satdump::user_path + "/osm_tiles/");

                    ImGui::EndTable();
                }
            }

            if (ImGui::CollapsingHeader("File Input/Output"))
            {
                if (ImGui::BeginTable("##satdumpoutput_directories", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
                {
                    for (std::pair<std::string, satdump::params::EditableParameter> &p : settings_output_directories)
                        p.second.draw();
                    ImGui::EndTable();
                }
            }

            if (config::plugin_config_handlers.size() > 0)
            {
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10 * ui_scale);
                ImGui::SeparatorText("Plugin Settings");
                for (auto &plugin_hdl : config::plugin_config_handlers)
                {
                    if (ImGui::CollapsingHeader(plugin_hdl.name.c_str()))
                    {
                        plugin_hdl.render();
                    }
                }
            }

            if (advanced_mode)
            {
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10 * ui_scale);
                ImGui::SeparatorText("Advanced Settings");
                if (ImGui::CollapsingHeader("TLE Settings"))
                {
                    widgets::JSONTreeEditor(satdump::config::main_cfg["tle_settings"], "tle_settings", false);
                    if (ImGui::Button("Reset##tle_settings"))
                        satdump::config::main_cfg["tle_settings"] = satdump::config::master_cfg["tle_settings"];
                }
                if (ImGui::CollapsingHeader("Advanced Settings"))
                {
                    widgets::JSONTreeEditor(satdump::config::main_cfg["advanced_settings"], "advanced_settings");
                    ImGui::SameLine();
                    if (ImGui::Button("Reset##advanced_settings"))
                        satdump::config::main_cfg["advanced_settings"] = satdump::config::master_cfg["advanced_settings"];
                }
                if (ImGui::CollapsingHeader("Instrument Config"))
                {
                    widgets::JSONTreeEditor(satdump::config::main_cfg["viewer"]["instruments"], "instrument_settings");
                    ImGui::SameLine();
                    if (ImGui::Button("Reset##instrument_settings"))
                        satdump::config::main_cfg["viewer"]["instruments"] = satdump::config::master_cfg["viewer"]["instruments"];
                }
                if (ImGui::CollapsingHeader("Default Pipeline Configs"))
                {
                    widgets::JSONTreeEditor(pipelines_json, "pipelines");
                    ImGui::SameLine();
                    if (ImGui::Button("Reset##pipelines"))
                        pipelines_json = pipelines_system_json;
                }
            }

            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5 * ui_scale);
            if (ImGui::Button("Save"))
            {
#ifdef USE_OPENCL
                satdump::config::main_cfg["satdump_general"]["opencl_device"]["platform"] = opencl_devices_enum[opencl_devices_id].platform_id;
                satdump::config::main_cfg["satdump_general"]["opencl_device"]["device"] = opencl_devices_enum[opencl_devices_id].device_id;
                opencl::resetOCLContext();
#endif

                for (std::pair<std::string, satdump::params::EditableParameter> &p : settings_user_interface)
                    satdump::config::main_cfg["user_interface"][p.first]["value"] = p.second.getValue();
                for (std::pair<std::string, satdump::params::EditableParameter> &p : settings_general)
                    satdump::config::main_cfg["satdump_general"][p.first]["value"] = p.second.getValue();
                for (std::pair<std::string, satdump::params::EditableParameter> &p : settings_output_directories)
                    satdump::config::main_cfg["satdump_directories"][p.first]["value"] = p.second.getValue();

                satdump::config::main_cfg["user_interface"]["theme"]["value"] = themes[selected_theme];

                for (auto &plugin_hdl : config::plugin_config_handlers)
                    plugin_hdl.save();

                config::saveUserConfig();
                if (advanced_mode)
                    savePipelines();
                advanced_mode = getValueOrDefault(satdump::config::main_cfg["user_interface"]["advanced_mode"]["value"], false);
                saved_message.set_message(style::theme.green, "Settings saved");
                satdump::update_ui = true;
            }

            saved_message.draw();
            ImGui::TextColored(style::theme.yellow, "Note : Some settings will require SatDump to be restarted\nto take effect!");
        }
    }
}
