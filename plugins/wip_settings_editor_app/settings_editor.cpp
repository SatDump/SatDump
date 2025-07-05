#include "common/utils.h"
#include "settings_editor.h"
#include "resources.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>

namespace satdump {

    WipSettingsEditorHandler::WipSettingsEditorHandler() 
    {
        handler_tree_icon = u8"\uf471";
    }

    WipSettingsEditorHandler::~WipSettingsEditorHandler()
    {
        json_files.clear();
        preset_names.clear();
        preset_selected.clear();
        original_jsons.clear();
        all_instruments.clear();
        enabled_instruments.clear();

        export_success    = false;
        export_status_msg.clear();
        instrument_to_add = -1;
        selected_dropdown = -1;
        std::memset(export_dir, 0, sizeof(export_dir));
    }

    void WipSettingsEditorHandler::drawMenu()
    {
        if (ImGui::CollapsingHeader("Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
            // Add settings options here
        }
    }

    void WipSettingsEditorHandler::drawMenuBar()
    {
        // file_open_menu.render("initiate the gizmo", "Select JSON file", ".", {{"JSON Files", ".json"}});
    }

    void WipSettingsEditorHandler::drawContents(ImVec2 win_size)
    {
        (void)win_size;
        // Load JSON files
        if (json_files.empty()) {
            auto dir = resources::getResourcePath("instrument_cfgs");

            for (const auto& entry : std::filesystem::directory_iterator(dir)) {
                if (!entry.is_regular_file() || entry.path().extension() != ".json")
                    continue;

                std::string filename = entry.path().stem().string();
                json_files.push_back(filename);
                all_instruments.push_back(filename);

                std::ifstream file(entry.path());
                std::stringstream buffer;
                buffer << file.rdbuf();

                ordered_json j;
                try {
                    j = ordered_json::parse(buffer.str());
                } catch (const std::exception&) {
                    // ignore parse errors
                }

                std::vector<std::string> names;
                std::vector<bool>        selected;

                if (j.contains("presets") && j["presets"].is_array()) {
                    for (const auto& pr : j["presets"]) {
                        if (pr.contains("name") && pr["name"].is_string()) {
                            names.push_back(pr["name"].get<std::string>());
                            selected.push_back(false);
                        }
                    }
                }

                preset_names.push_back(std::move(names));
                preset_selected.push_back(std::move(selected));
                original_jsons.push_back(std::move(j));
            }
        }

        // UI for export directory and buttons
        ImGui::Text("Export Selected Presets:");
        ImGui::InputText("", export_dir, sizeof(export_dir));
        ImGui::SameLine();

        if (ImGui::Button("Export Configs")) {
            export_success    = false;
            export_status_msg.clear();

            if (std::strlen(export_dir) == 0) {
                export_status_msg = "Please specify an export directory.";
            } else {
                std::filesystem::path out_dir(export_dir);
                try {
                    if (!std::filesystem::exists(out_dir)) {
                        std::filesystem::create_directories(out_dir);
                    }

                    for (size_t i = 0; i < json_files.size(); ++i) {
                        if (std::find(enabled_instruments.begin(),
                                    enabled_instruments.end(),
                                    json_files[i]) == enabled_instruments.end())
                        {
                            continue;
                        }

                        ordered_json export_json = original_jsons[i];
                        if (export_json.contains("presets") && export_json["presets"].is_array()) {
                            ordered_json filtered = ordered_json::array();

                            for (size_t j = 0; j < preset_names[i].size(); ++j) {
                                if (!preset_selected[i][j])
                                    continue;

                                for (const auto& pr : export_json["presets"]) {
                                    if (pr.contains("name") && pr["name"].is_string() &&
                                        pr["name"].get<std::string>() == preset_names[i][j])
                                    {
                                        filtered.push_back(pr);
                                        break;
                                    }
                                }
                            }

                            export_json["presets"] = filtered;
                        }

                        auto out_file = out_dir / (json_files[i] + ".json");
                        std::ofstream ofs(out_file);
                        if (!ofs) {
                            export_status_msg += "Failed to write: " + out_file.string() + "\n";
                        } else {
                            ofs << export_json.dump(4);
                        }
                    }

                    if (export_status_msg.empty()) {
                        export_success    = true;
                        export_status_msg = "Export completed successfully.";
                    }
                } catch (const std::exception& e) {
                    export_status_msg = std::string("Export failed: ") + e.what();
                }
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Export to Config")) {
            export_config_pressed = true;
            /* 
            //functionality not implemented yet - once satdump supports config overrride this will be modified
            
            export_success    = false;
            export_status_msg.clear();
            
            try {
                ordered_json config_export = ordered_json::array();
            
                for (size_t i = 0; i < json_files.size(); ++i) {
                    if (std::find(enabled_instruments.begin(),
                                enabled_instruments.end(),
                                json_files[i]) == enabled_instruments.end())
                    {
                        continue;
                    }
            
                    const auto& input = original_jsons[i];
                    ordered_json out_obj;
            
                    if (input.contains("name")) {
                        out_obj["name"] = input["name"];
                    }
            
                    if (input.contains("presets") && input["presets"].is_array()) {
                        ordered_json filtered = ordered_json::array();
            
                        for (size_t j = 0; j < preset_names[i].size(); ++j) {
                            if (!preset_selected[i][j])
                                continue;
            
                            for (const auto& pr : input["presets"]) {
                                if (pr.contains("name") && pr["name"].is_string() &&
                                    pr["name"].get<std::string>() == preset_names[i][j])
                                {
                                    filtered.push_back(pr);
                                    break;
                                }
                            }
                        }
            
                        out_obj["presets"] = filtered;
                    }
            
                    if (input.contains("default")) {
                        out_obj["default"] = input["default"];
                    }
            
                    for (auto& [key, val] : input.items()) {
                        if (key != "name" && key != "presets" && key != "default") {
                            out_obj[key] = val;
                        }
                    }
            
                    config_export.push_back(std::move(out_obj));
                }
            
                constexpr auto path = "config/";
                std::ofstream ofs(path);
            
                if (!ofs) {
                    export_status_msg = std::string("Failed to write: ") + path;
                } else {
                    ofs << config_export.dump(4);
                    export_success    = true;
                    export_status_msg = std::string("Exported to ") + path;
                }
            } catch (const std::exception& e) {
                export_status_msg = std::string("Export failed: ") + e.what();
            }
            */
        }
        if (export_config_pressed) {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "WIP feature - not currently possible");
        }

        if (!export_status_msg.empty()) {
            ImVec4 color = export_success ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0, 0, 1);
            ImGui::TextColored(color, "%s", export_status_msg.c_str());
        }

        ImGui::Separator();

        // --- Instrument selection ---
        std::vector<std::string> available_instruments;
        for (const auto& inst : all_instruments) {
            if (std::find(enabled_instruments.begin(), enabled_instruments.end(), inst) ==
                enabled_instruments.end())
            {
                available_instruments.push_back(inst);
            }
        }

        std::sort(available_instruments.begin(), available_instruments.end());

        ImGui::Text("Add Instrument:");
        if (available_instruments.empty()) {
            ImGui::TextDisabled("None available");
        } else {
            if (ImGui::BeginCombo("##instrument_dropdown",
                                selected_dropdown >= 0
                                    ? available_instruments[selected_dropdown].c_str()
                                    : "None"))
            {
                for (int n = 0; n < static_cast<int>(available_instruments.size()); ++n) {
                    bool is_selected = (selected_dropdown == n);
                    if (ImGui::Selectable(available_instruments[n].c_str(), is_selected)) {
                        selected_dropdown = n;
                    }
                    if (is_selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::SameLine();
            if (ImGui::Button("Add") && selected_dropdown >= 0) {
                enabled_instruments.push_back(available_instruments[selected_dropdown]);
                selected_dropdown = -1;
            }

            ImGui::SameLine();
            if (ImGui::Button("Add All")) {
                for (const auto& inst : available_instruments) {
                    enabled_instruments.push_back(inst);
                }
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Remove All")) {
            enabled_instruments.clear();
        }

        ImGui::Separator();

        ImGui::Text("Current instrument configs to be exported:");
        ImGui::BeginChild("json_files_scroll", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);

        for (size_t i = 0; i < json_files.size(); ++i) {
            const auto& name = json_files[i];
            if (std::find(enabled_instruments.begin(), enabled_instruments.end(), name) ==
                enabled_instruments.end())
            {
                continue;
            }

            ImGui::PushID(static_cast<int>(i));
            float full_w   = ImGui::GetContentRegionAvail().x;
            float button_w = ImGui::CalcTextSize("Remove").x + ImGui::GetStyle().FramePadding.x * 2;
            float header_w = full_w - button_w - ImGui::GetStyle().ItemSpacing.x;

            ImGui::Columns(2, nullptr, false);
            ImGui::SetColumnWidth(0, header_w);

            bool open = ImGui::CollapsingHeader((name + "##" + std::to_string(i)).c_str());
            ImGui::NextColumn();

            if (ImGui::Button("Remove", ImVec2(button_w, 0))) {
                enabled_instruments.erase(
                    std::remove(enabled_instruments.begin(), enabled_instruments.end(), name),
                    enabled_instruments.end());
                ImGui::Columns(1);
                ImGui::PopID();
                continue;
            }

            ImGui::Columns(1);

            if (open) {
                if (ImGui::Button("Select All")) {
                    std::fill(preset_selected[i].begin(), preset_selected[i].end(), true);
                }
                ImGui::SameLine();
                if (ImGui::Button("Select None")) {
                    std::fill(preset_selected[i].begin(), preset_selected[i].end(), false);
                }

                for (size_t j = 0; j < preset_names[i].size(); ++j) {
                    bool checked = preset_selected[i][j];
                    if (ImGui::Checkbox(
                            (preset_names[i][j] + "##" + std::to_string(i) + "_" + std::to_string(j)).c_str(),
                            &checked))
                    {
                        preset_selected[i][j] = checked;
                    }
                }
            }

            ImGui::PopID();
        }

        ImGui::EndChild();
    }

}
