#include "product_handler.h"
#include "resources.h"
#include "logger.h"
#include "nlohmann/json_utils.h"
#include "common/utils.h"
#include "core/exception.h"
#include "imgui/imgui_stdlib.h"

namespace satdump
{
    namespace viewer
    {
        ProductHandler::ProductHandler(std::shared_ptr<products::Product> p, bool dataset_mode)
            : product(p)
        {
            handler_tree_icon = "\uf713";

            std::string config_path = "instrument_cfgs/" + product->instrument_name + ".json";
            if (resources::resourceExists(config_path))
            {
                instrument_cfg = loadJsonFile(resources::getResourcePath(config_path));
                if (instrument_cfg.contains("name"))
                    handler_name = instrument_cfg["name"];
            }
            else
            {
                logger->warn("Couldn't open instrument configuration at " + config_path + ". Expect degraded experience.");
                handler_name = product->instrument_name;
            }

            if (!dataset_mode && p->has_product_source())
                handler_name = p->get_product_source() + " " + handler_name;
            if (!dataset_mode && p->has_product_timestamp())
                handler_name += " " + timestamp_to_string(p->get_product_timestamp());

            // TODOREWORK, handle automated?, Filtering what can be made per channels present?
            if (instrument_cfg.contains("presets"))
            {
                for (auto &cfg : instrument_cfg["presets"])
                {
                    try
                    {
                        preset_selection_box_str.push_back(cfg["name"].get<std::string>());
                    }
                    catch (std::exception &e)
                    {
                        logger->error("Error loading preset! JSON : " + cfg.dump());
                    }
                }
            }
        }

        bool ProductHandler::renderPresetMenu()
        {
            bool was_changed = false;

            if (ImGui::CollapsingHeader("Presets", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (ImGui::BeginCombo("##presetproductcombo", /*TODOREWORK maybe make this generic?*/
                                      (preset_selection_curr_id >= 0 && preset_selection_curr_id < preset_selection_box_str.size())
                                          ? preset_selection_box_str[preset_selection_curr_id].c_str()
                                          : ""))
                {
                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                    ImGui::InputTextWithHint("##searchpresets", u8"\uf422   Search", &preset_search_str);
                    for (size_t i = 0; i < preset_selection_box_str.size(); i++)
                    {
                        bool show = true;
                        if (preset_search_str.size() != 0)
                            show = isStringPresent(preset_selection_box_str[i], preset_search_str);

                        if (show && ImGui::Selectable(preset_selection_box_str[i].c_str(), (int)i == preset_selection_curr_id))
                        {
                            preset_selection_curr_id = i;

                            auto preset = instrument_cfg["presets"][preset_selection_curr_id];
                            setConfig(preset);
                            was_changed = true;
                            preset_selection_curr_id = -1;

                            // Description, optional
                            if (preset.contains("description"))
                            {
                                std::ifstream ifs(resources::getResourcePath(preset["description"]));
                                std::string desc_markdown((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
                                markdown_info.set_md(desc_markdown);
                                has_markdown_description = true;
                            }
                            else
                            {
                                has_markdown_description = false;
                            }
                        }
                    }
                    ImGui::EndCombo();
                }

                ImGui::SameLine();
                if (ImGui::Button("Default"))
                {
                    tryApplyDefaultPreset();
                    was_changed = true;
                }
                if (has_markdown_description)
                {
                    ImGui::SameLine();
                    if (ImGui::Button(u8"\uf449###resetinfo"))
                        show_markdown_description = true;
                }
            }

            // Show description window
            if (show_markdown_description)
            {
                ImGuiIO &io = ImGui::GetIO();
                ImGui::SetNextWindowSize({400 * ui_scale, 400 * ui_scale}, ImGuiCond_Appearing);
                ImGui::SetNextWindowPos(ImVec2((io.DisplaySize.x / 2) - (400 * ui_scale / 2), (io.DisplaySize.y / 2) - (400 * ui_scale / 2)), ImGuiCond_Appearing);
                ImGui::Begin("Preset Info", &show_markdown_description, ImGuiWindowFlags_NoSavedSettings);
                markdown_info.render();
                ImGui::End();
            }

            return was_changed;
        }

        void ProductHandler::tryApplyDefaultPreset()
        {
            if (instrument_cfg.contains("default"))
                setConfig(instrument_cfg["default"]);
        }

        void ProductHandler::saveResult(std::string directory)
        {
            throw satdump_exception("saveResult NOT implemented for this handler!");
        }
    }
}