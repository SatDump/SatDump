#include "imgui/imgui.h"
#include "imgui/imgui_stdlib.h"
#include "core/config.h"
#include "core/style.h"

#include "dcs_settings.h"

namespace goes
{
    namespace hrit
    {
        static const char update_interval_options[] = "4 hours\0" "1 day\0" "3 Days\0" "7 Days\0" "14 Days\0" "Never\0\0";
        static int update_interval_selection = 0;
        static bool *advanced_mode;

        static std::vector<std::string> pdt_urls =
        {
            "https://dcs1.noaa.gov/PDTS_COMPRESSED.txt",
            "https://dcs2.noaa.gov/PDTS_COMPRESSED.txt",
            "https://dcs3.noaa.gov/PDTS_COMPRESSED.txt",
            "https://dcs4.noaa.gov/PDTS_COMPRESSED.txt"
        };

        static std::vector<std::string> hads_urls =
        {
            "https://hads.ncep.noaa.gov/compressed_defs/all_dcp_defs.txt"
        };

        void initDcsConfig()
        {
            advanced_mode = satdump::config::main_cfg["user_interface"]["advanced_mode"]["value"].get_ptr<nlohmann::json::boolean_t*>();
            if (!satdump::config::main_cfg["plugin_settings"].contains("goes_support"))
                satdump::config::main_cfg["plugin_settings"]["goes_support"] = nlohmann::ordered_json();

            // Initialize last update
            if (!satdump::config::main_cfg["plugin_settings"]["goes_support"].contains("last_pdt_update") ||
                !satdump::config::main_cfg["plugin_settings"]["goes_support"]["last_pdt_update"].is_number())
                satdump::config::main_cfg["plugin_settings"]["goes_support"]["last_pdt_update"] = 0;

            // Initialize Update Interval
            if (satdump::config::main_cfg["plugin_settings"]["goes_support"].contains("update_interval") &&
                satdump::config::main_cfg["plugin_settings"]["goes_support"]["update_interval"].is_number())
            {
                int update_interval = satdump::config::main_cfg["plugin_settings"]["goes_support"]["update_interval"];
                if (update_interval == 14400)       // 4 hours
                    update_interval_selection = 0;
                else if (update_interval == 86400)  // 1 day
                    update_interval_selection = 1;
                else if (update_interval == 259200) // 3 days
                    update_interval_selection = 2;
                else if (update_interval == 604800) // 7 days
                    update_interval_selection = 3;
                else if (update_interval == -1)     // Never
                    update_interval_selection = 4;
            }
            else
                satdump::config::main_cfg["plugin_settings"]["goes_support"]["update_interval"] = 14400;

            // Initialize PDT URLs
            if (satdump::config::main_cfg["plugin_settings"]["goes_support"].contains("pdt_urls") &&
                satdump::config::main_cfg["plugin_settings"]["goes_support"]["pdt_urls"].is_array())
                pdt_urls = satdump::config::main_cfg["plugin_settings"]["goes_support"]["pdt_urls"];
            else
                satdump::config::main_cfg["plugin_settings"]["goes_support"]["pdt_urls"] = pdt_urls;

            // Initialize HADS URLs
            if (satdump::config::main_cfg["plugin_settings"]["goes_support"].contains("hads_urls") &&
                satdump::config::main_cfg["plugin_settings"]["goes_support"]["hads_urls"].is_array())
                hads_urls = satdump::config::main_cfg["plugin_settings"]["goes_support"]["hads_urls"];
            else
                satdump::config::main_cfg["plugin_settings"]["goes_support"]["hads_urls"] = hads_urls;
        }

        void renderDcsConfig()
        {
            ImGuiStyle style = ImGui::GetStyle();
            ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(style.CellPadding.x, 5.0 * ui_scale));
            if (ImGui::BeginTable("##satdumpgoesdcssettings", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted("PDT/HADS Update Interval");
                ImGui::SetItemTooltip("%s", "PDT and HADS define information about DCPs around the world necessary for decoding.\nTo manage download URLs, enable advanced mode.");
                ImGui::TableSetColumnIndex(1);
                ImGui::Combo("##goesdcsupdateinterval", &update_interval_selection, update_interval_options);

                if (*advanced_mode)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted("PDT URLs");
                    ImGui::TableSetColumnIndex(1);
                    int to_remove = -1;
                    for (size_t i = 0; i < pdt_urls.size(); i++)
                    {
                        ImGui::InputText(std::string("##pdturl" + std::to_string(i)).c_str(), &pdt_urls[i]);
                        ImGui::SameLine();
                        if (ImGui::Button(std::string("Remove##pdt" + std::to_string(i)).c_str()))
                            to_remove = i;
                    }
                    if (to_remove != -1)
                        pdt_urls.erase(pdt_urls.begin() + to_remove);
                    if (ImGui::Button("Add##pdtaddurl"))
                        pdt_urls.push_back("");

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted("HADS URLs");
                    ImGui::TableSetColumnIndex(1);
                    to_remove = -1;
                    for (size_t i = 0; i < hads_urls.size(); i++)
                    {
                        ImGui::InputText(std::string("##hadsurl" + std::to_string(i)).c_str(), &hads_urls[i]);
                        ImGui::SameLine();
                        if (ImGui::Button(std::string("Remove##pdt" + std::to_string(i)).c_str()))
                            to_remove = i;
                    }
                    if (to_remove != -1)
                        hads_urls.erase(hads_urls.begin() + to_remove);
                    if (ImGui::Button("Add##hadsaddurl"))
                        hads_urls.push_back("");
                }

                ImGui::EndTable();
            }
            ImGui::PopStyleVar();

            ImGui::Spacing();
            ImGui::TextUnformatted("Note:");
            ImGui::SameLine();
            ImGui::TextDisabled("%s", "These Settings are only enabled if Parse DCS is enabled on the GOES-R HRIT pipeline");
            ImGui::Spacing();
        }

        void saveDcsConfig()
        {
            satdump::config::main_cfg["plugin_settings"]["goes_support"]["pdt_urls"] = pdt_urls;
            satdump::config::main_cfg["plugin_settings"]["goes_support"]["hads_urls"] = hads_urls;

            if (update_interval_selection == 0)      // 4 hours
                satdump::config::main_cfg["plugin_settings"]["goes_support"]["update_interval"] = 14400;
            else if (update_interval_selection == 1) // 1 day
                satdump::config::main_cfg["plugin_settings"]["goes_support"]["update_interval"] = 86400;
            else if (update_interval_selection == 2) // 3 days
                satdump::config::main_cfg["plugin_settings"]["goes_support"]["update_interval"] = 259200;
            else if (update_interval_selection == 3) // 7 days
                satdump::config::main_cfg["plugin_settings"]["goes_support"]["update_interval"] = 604800;
            else if (update_interval_selection == 4) // Never
                satdump::config::main_cfg["plugin_settings"]["goes_support"]["update_interval"] = -1;
        }
    }
}