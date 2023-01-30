#include "settings.h"
#include "imgui/imgui.h"
#include <string>
#include "core/params.h"

#include "core/config.h"

#include "main_ui.h"
#include "core/opencl.h"

#include "init.h"
#include "common/tracking/tle.h"

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

        bool tles_are_update = false;

        bool show_imgui_demo = false;

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

            params = satdump::config::main_cfg["satdump_output_directories"];

            for (nlohmann::detail::iteration_proxy_value<nlohmann::detail::iter_impl<nlohmann::ordered_json>> cfg : params.items())
            {
                // Check setting type, and create an EditableParameter if possible
                if (cfg.value().contains("type") && cfg.value().contains("value") && cfg.value().contains("name"))
                    settings_output_directories.push_back({cfg.key(), params::EditableParameter(nlohmann::json(cfg.value()))});
            }

#ifdef USE_OPENCL
            opencl_devices_enum = opencl::getAllDevices();
            int p = satdump::config::main_cfg["satdump_general"]["opencl_device"]["platform"].get<int>();
            int d = satdump::config::main_cfg["satdump_general"]["opencl_device"]["device"].get<int>();
            int dev_id = 0;
            opencl_devices_str = "";
            for (opencl::OCLDevice &dev : opencl_devices_enum)
            {
                if (dev_id == (int)opencl_devices_enum.size() - 1)
                    opencl_devices_str += dev.name;
                else
                    opencl_devices_str += dev.name + " \0";
                if (dev.platform_id == p && dev.device_id == d)
                    opencl_devices_id = dev_id;
                dev_id++;
            }
#endif
        }

        void render()
        {
            if (ImGui::CollapsingHeader("User Interface"))
            {
                if (ImGui::BeginTable("##satdumpuisettings", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
                {
                    for (std::pair<std::string, satdump::params::EditableParameter> &p : settings_user_interface)
                        p.second.draw();

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
                    if (tles_are_update)
                        style::beginDisabled();
                    if (ImGui::Button("Update###updateTLEs"))
                    {
                        ui_thread_pool.push([](int)
                                            {   tles_are_update = true;
                                                updateTLEFile(satdump::user_path + "/satdump_tles.txt"); 
                                                loadTLEFileIntoRegistry(satdump::user_path + "/satdump_tles.txt"); 
                                                tles_are_update = false; });
                    }
                    if (tles_are_update)
                        style::endDisabled();

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

            if (ImGui::CollapsingHeader("Output Directories"))
            {
                if (ImGui::BeginTable("##satdumpoutput_directories", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
                {
                    for (std::pair<std::string, satdump::params::EditableParameter> &p : settings_output_directories)
                        p.second.draw();
                    ImGui::EndTable();
                }
            }

            if (ImGui::Button("Save"))
            {
#ifdef USE_OPENCL
                if (opencl_devices_enum.size() > 0)
                {
                    satdump::config::main_cfg["satdump_general"]["opencl_device"]["platform"] = opencl_devices_enum[opencl_devices_id].platform_id;
                    satdump::config::main_cfg["satdump_general"]["opencl_device"]["device"] = opencl_devices_enum[opencl_devices_id].device_id;
                }
#endif

                for (std::pair<std::string, satdump::params::EditableParameter> &p : settings_user_interface)
                    satdump::config::main_cfg["user_interface"][p.first]["value"] = p.second.getValue();
                for (std::pair<std::string, satdump::params::EditableParameter> &p : settings_general)
                    satdump::config::main_cfg["satdump_general"][p.first]["value"] = p.second.getValue();
                for (std::pair<std::string, satdump::params::EditableParameter> &p : settings_output_directories)
                    satdump::config::main_cfg["satdump_output_directories"][p.first]["value"] = p.second.getValue();
                config::saveUserConfig();
            }

            ImGui::TextColored(ImColor(255, 255, 0), "Note : Some settings will require SatDump to be restarted\nto take effect!");
        }
    }
}
