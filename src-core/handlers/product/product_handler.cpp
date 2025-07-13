#include "product_handler.h"
#include "core/config.h"
#include "core/exception.h"
#include "core/resources.h"
#include "imgui/imgui_stdlib.h"
#include "logger.h"
#include "nlohmann/json.hpp"
#include "nlohmann/json_utils.h"
#include "utils/string.h"
#include "utils/time.h"
#include <regex>

namespace satdump
{
    namespace handlers
    {
        ProductHandler::ProductHandler(std::shared_ptr<products::Product> p, bool dataset_mode, std::function<bool(nlohmann::ordered_json &)> filterPreset) : product(p)
        {
            handler_tree_icon = u8"\uf713";

            std::string config_path = "instrument_cfgs/" + product->instrument_name + ".json";
            if (resources::resourceExists(config_path))
            {
                try
                {
                    instrument_cfg = loadJsonFile(resources::getResourcePath(config_path));
                    if (instrument_cfg.contains("name"))
                        handler_name = instrument_cfg["name"];

                    std::filesystem::recursive_directory_iterator commonIterator(resources::getResourcePath("instrument_cfgs/common"));
                    std::error_code iteratorError;
                    while (commonIterator != std::filesystem::recursive_directory_iterator())
                    {
                        std::string path = commonIterator->path().string();
                        if (std::filesystem::is_regular_file(commonIterator->path()))
                        {
                            try
                            {
                                auto additional_presets = loadJsonFile(path);
                                auto instr = additional_presets["instruments"].get<std::vector<std::string>>();

                                if (std::find_if(instr.begin(), instr.end(), [&](const std::string &el) { return el == p->instrument_name; }) != instr.end())
                                    for (nlohmann::ordered_json p : additional_presets["presets"])
                                        instrument_cfg["presets"].push_back(p);
                            }
                            catch (std::exception &e)
                            {
                                logger->error("Common instrument configuration (" + path + ") invalid! %s", e.what());
                            }
                        }

                        commonIterator.increment(iteratorError);
                        if (iteratorError)
                            logger->critical(iteratorError.message());
                    }
                }
                catch (std::exception &e)
                {
                    logger->error("Instrument configuration invalid! %s", e.what());
                }
            }
            else
            {
                logger->warn("Couldn't open instrument configuration at " + config_path + ". Expect degraded experience.");
                handler_name = product->instrument_name;
            }

            // Sort in alphabetical order (to make lego11 happy)
            if (!instrument_cfg["presets"].is_null())
                std::sort(instrument_cfg["presets"].begin(), instrument_cfg["presets"].end(),
                          [&](const nlohmann::ordered_json &a, const nlohmann::ordered_json &b) { return a["name"].get<std::string>() < b["name"].get<std::string>(); });

            if (!dataset_mode && p->has_product_source())
                handler_name = p->get_product_source() + " " + handler_name;
            if (!dataset_mode && p->has_product_timestamp())
                handler_name += " " + timestamp_to_string(p->get_product_timestamp());
            if (p->has_product_id())
                handler_name += " (" + p->get_product_id() + ")";

            // TODOREWORK, handle automated?, Filtering what can be made per channels present?
            if (instrument_cfg.contains("presets"))
            {
                for (auto &cfg : instrument_cfg["presets"])
                {
                    try
                    {
                        if (filterPreset(cfg))
                        {
                            preset_selection_box_str.push_back(cfg["name"].get<std::string>());
                            all_presets.push_back(cfg);
                        }
                        else
                        {
                            logger->trace("Disabling unavailable preset : " + cfg["name"].get<std::string>());
                        }
                    }
                    catch (std::exception &e)
                    {
                        if (cfg.contains("name"))
                            logger->warn("Error loading preset (" + cfg["name"].get<std::string>() + ")! JSON : " + cfg.dump());
                        else
                            logger->warn("Error loading preset! JSON : " + cfg.dump());
                    }
                }
            }
        }

        bool ProductHandler::renderPresetMenu()
        {
            bool was_changed = false;

            if (ImGui::CollapsingHeader("Presets", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (ImGui::BeginCombo("##presetproductcombo", (preset_selection_curr_id >= 0 && preset_selection_curr_id < preset_selection_box_str.size())
                                                                  ? preset_selection_box_str[preset_selection_curr_id].c_str()
                                                                  : "Select Preset..."))
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

                            auto preset = all_presets[preset_selection_curr_id];
                            setConfig(preset);
                            was_changed = true;
                            if (!preset_reset_by_handler) // If this is handled by the handler implementing it, don't auto-reset
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

        std::string ProductHandler::generateFileName()
        {
            std::string file_name = satdump_cfg.main_cfg["satdump_directories"]["image_filename_template"]["value"].get<std::string>();
            std::string default_path = satdump_cfg.main_cfg["satdump_directories"]["default_image_output_directory"]["value"].get<std::string>();
            time_t timevalue = 0;

            std::string product_source = product->has_product_source() ? product->get_product_source() : "Unknown";
            std::string product_name = product_internal_name; // TODOREWORK select_image_id != 0 ? ("ch" + channel_numbers[select_image_id - 1]) : (select_rgb_presets == -1 ? "custom" :
                                                              // rgb_presets[select_rgb_presets].first);
            if (product->has_product_timestamp())
                timevalue = product->get_product_timestamp();
            std::tm *timeReadable = gmtime(&timevalue);

            std::string instrument_name_upper, product_source_upper, product_name_upper, product_name_abbr;
            instrument_name_upper.resize(product->instrument_name.size());
            product_source_upper.resize(product_source.size());
            product_name_upper.resize(product_name.size());
            std::transform(product->instrument_name.begin(), product->instrument_name.end(), instrument_name_upper.begin(), ::toupper);
            std::transform(product_source.begin(), product_source.end(), product_source_upper.begin(), ::toupper);
            std::transform(product_name.begin(), product_name.end(), product_name_upper.begin(), ::toupper);
            for (char &name_char : product_name)
                if ((name_char >= 'A' && name_char <= 'Z') || (name_char >= '-' && name_char <= '9'))
                    product_name_abbr += name_char;
            std::replace(instrument_name_upper.begin(), instrument_name_upper.end(), ' ', '_');
            std::replace(product_source_upper.begin(), product_source_upper.end(), ' ', '_');
            std::replace(product_name_upper.begin(), product_name_upper.end(), ' ', '_');

            file_name = std::regex_replace(file_name, std::regex("\\$t"), std::to_string(timevalue));
            file_name = std::regex_replace(file_name, std::regex("\\$y"), std::to_string(timeReadable->tm_year + 1900));
            file_name = std::regex_replace(file_name, std::regex("\\$M"), timeReadable->tm_mon + 1 > 9 ? std::to_string(timeReadable->tm_mon + 1) : "0" + std::to_string(timeReadable->tm_mon + 1));
            file_name = std::regex_replace(file_name, std::regex("\\$d"), timeReadable->tm_mday > 9 ? std::to_string(timeReadable->tm_mday) : "0" + std::to_string(timeReadable->tm_mday));
            file_name = std::regex_replace(file_name, std::regex("\\$h"), timeReadable->tm_hour > 9 ? std::to_string(timeReadable->tm_hour) : "0" + std::to_string(timeReadable->tm_hour));
            file_name = std::regex_replace(file_name, std::regex("\\$m"), timeReadable->tm_min > 9 ? std::to_string(timeReadable->tm_min) : "0" + std::to_string(timeReadable->tm_min));
            file_name = std::regex_replace(file_name, std::regex("\\$s"), timeReadable->tm_sec > 9 ? std::to_string(timeReadable->tm_sec) : "0" + std::to_string(timeReadable->tm_sec));
            file_name = std::regex_replace(file_name, std::regex("\\$i"), product->instrument_name);
            file_name = std::regex_replace(file_name, std::regex("\\$I"), instrument_name_upper);
            file_name = std::regex_replace(file_name, std::regex("\\$c"), product_name);
            file_name = std::regex_replace(file_name, std::regex("\\$C"), product_name_upper);
            file_name = std::regex_replace(file_name, std::regex("\\$a"), product_name_abbr);
            file_name = std::regex_replace(file_name, std::regex("\\$o"), product_source);
            file_name = std::regex_replace(file_name, std::regex("\\$O"), product_source_upper);

            // Make sure we never overwrite anything unintentionally
            std::string filename_suffix = "";
            int suffix_num = 1;
            while (std::filesystem::exists(default_path + "/" + file_name + filename_suffix))
                filename_suffix = "_" + std::to_string(++suffix_num);
            file_name += filename_suffix;

            // Remove common problematic characters
            std::replace(file_name.begin(), file_name.end(), '/', '-');
            file_name = std::regex_replace(file_name, std::regex(u8"\u00B5"), u8"u");
            file_name = std::regex_replace(file_name, std::regex(u8"\u03BB="), u8"");

            return file_name;
        }

        void ProductHandler::saveResult(std::string directory) { throw satdump_exception("saveResult NOT implemented for this handler! => " + directory); }
    } // namespace handlers
} // namespace satdump