#include "image_product_handler.h"
#include "imgui/imgui.h"
#include "nlohmann/json_utils.h"

#include "core/style.h"
#include "imgui/imgui_stdlib.h"
#include "logger.h"

#include "common/dsp_source_sink/format_notated.h" // TODOREWORK
#include "imgui/imgui_stdlib.h"
#include "products/image/calibration_converter.h"
#include "products/image/product_expression.h"

#include "common/widgets/json_editor.h"
#include "products/image/calibration_units.h" // TODOREWORK
#include <cstddef>
#include <string>

namespace satdump
{
    namespace handlers
    {
        ImageProductHandler::ImageProductHandler(std::shared_ptr<products::Product> p, bool dataset_mode)
            : ProductHandler(p, dataset_mode, [p](auto &c) { return products::check_expression_product_composite((products::ImageProduct *)p.get(), c["expression"]); })
        {
            handler_tree_icon = u8"\uf71e";
            preset_reset_by_handler = true;

            product = (products::ImageProduct *)ProductHandler::product.get();
            for (auto &img : product->images)
                channel_selection_box_str += "Channel " + img.channel_name + '\0';

            // Try to init calibration
            img_calibrator = products::get_calibrator_from_product(product);
            if (img_calibrator)
                images_can_be_calibrated = true;

            // Init image handler used to display images
            img_handler = std::make_shared<ImageHandler>();

            // Mouse callback, showing raw counts & calibrated counts
            img_handler->additionalMouseCallback = [this](int x, int y)
            {
                if (is_processing)
                    return;

                ImGui::SeparatorText("Product Data");

                if (channel_selection_curr_id != -1)
                    ImGui::Text("Count : %d", product->get_raw_channel_val(channel_selection_curr_id, x, y));

                // Show all possible units, if the image is raw
                if (img_calibrator && channel_selection_curr_id != -1)
                {
                    double val = img_calibrator->compute(channel_selection_curr_id, x, y);
                    for (auto &u : channels_calibration_units_and_converters)
                    {
                        double vval = u.second->convert(x, y, val);
                        if (vval == CALIBRATION_INVALID_VALUE)
                            ImGui::Text("%s : Invalid Value", u.first.name.c_str());
                        else
                            ImGui::Text("%s : %f %s", u.first.name.c_str(), vval, u.first.unit.c_str());
                    }
                }
            };

            // Calib range init
            if (images_can_be_calibrated)
            {
                for (size_t i = 0; i < product->images.size(); i++)
                {
                    std::map<std::string, CalibInfo> units_ranges;

                    auto channel_calibrated_output_units = calibration::getAvailableConversions(product->images[i].calibration_type);
                    for (auto &u : channel_calibrated_output_units)
                        units_ranges.emplace(u, CalibInfo{calibration::getUnitInfo(u), 0.0, 100.0});

                    if ((int)i == channel_selection_curr_id && units_ranges.size() > 0)
                        channels_calibrated_curr_unit = units_ranges.begin()->first;

                    channels_calibrated_ranges.emplace(i, units_ranges);
                }

                initOverlayConverters();
            }

            tryApplyDefaultPreset();
        }

        ImageProductHandler::~ImageProductHandler() {}

        void ImageProductHandler::drawMenu()
        {
            bool needs_to_be_disabled = is_processing;

            if (ImGui::CollapsingHeader("Channels", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (needs_to_be_disabled)
                    style::beginDisabled();

                // Main channel selection combo. Setups calibration if available
                if (ImGui::Combo("##imageproductchannelcombo", &channel_selection_curr_id, channel_selection_box_str.c_str()))
                {
                    if (channel_selection_curr_id != -1)
                    {
                        if (channels_calibrated_ranges.count(channel_selection_curr_id) && channels_calibrated_ranges[channel_selection_curr_id].size() > 0)
                            channels_calibrated_curr_unit = channels_calibrated_ranges[channel_selection_curr_id].begin()->first;
                    }

                    initOverlayConverters();

                    needs_to_update = true;
                }

                // If possible, display physical channel info (wavelength etc)
                if (channel_selection_curr_id != -1)
                {
                    auto &ch = product->images[channel_selection_curr_id];

                    if (ch.wavenumber != -1)
                    {
                        auto freq = product->get_channel_frequency(ch.abs_index);
                        ImGui::SameLine();
                        ImGui::Text("(%s)", format_notated(SPEED_OF_LIGHT_M_S / freq, "m", 2).c_str());
                        ImGui::Text(u8"%.3f cm\u207b\u00b9 / %s", ch.wavenumber, format_notated(freq, "Hz", 2).c_str());
                    }
                }

                // Calibration menu, if possible. Allow editing ranges
                if (channel_selection_curr_id != -1 && images_can_be_calibrated)
                {
                    std::string &curr_unit = channels_calibrated_curr_unit;
                    if (channels_calibrated_ranges[channel_selection_curr_id].count(curr_unit))
                    {
                        auto &u = channels_calibrated_ranges[channel_selection_curr_id][curr_unit];

                        needs_to_update |= ImGui::Checkbox("Calibrate", &channel_calibrated);
                        ImGui::SetNextItemWidth(150 * ui_scale);
                        ImGui::InputDouble("##rangemin", &u.min);
                        ImGui::SameLine();
                        ImGui::SetNextItemWidth(150 * ui_scale);
                        ImGui::InputDouble("##rangemax", &u.max);

                        // List of all units
                        if (ImGui::BeginCombo("Unit##calibunit", channels_calibrated_ranges[channel_selection_curr_id][curr_unit].info.getNiceUnits().c_str()))
                        {
                            for (auto &lu : channels_calibrated_ranges[channel_selection_curr_id])
                            {
                                if (ImGui::Selectable(lu.second.info.getNiceUnits().c_str(), lu.first == curr_unit))
                                {
                                    curr_unit = lu.first;
                                    needs_to_update = true;
                                }
                            }
                            ImGui::EndCombo();
                        }

                        needs_to_update |= ImGui::Button("Update###updatecalib");
                        ImGui::SameLine();
                        if (ImGui::Button("Add To Equ###calibaddtoexpression"))
                            expression = "cch" + product->images[channel_selection_curr_id].channel_name + "=(" + product->images[channel_selection_curr_id].channel_name + ", " + curr_unit + ", " +
                                         std::to_string(u.min) + ", " + std::to_string(u.max) + ");\n" + expression;
                    }
                    else
                    {
                        ImGui::Text("Calibration Error. Please Report!");
                    }
                }

                if (needs_to_be_disabled)
                    style::endDisabled();
            }

            bool presetSet = false;
            needs_to_update |= presetSet = renderPresetMenu();

            if (ImGui::CollapsingHeader("Expression", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (needs_to_be_disabled)
                    style::beginDisabled();

                // Expression entry
                ImGui::SetNextItemWidth(ImGui::GetWindowSize().x - 10 * ui_scale);
                ImGui::InputTextMultiline("##expression", &expression);
                if (ImGui::Button("Apply"))
                {
                    channel_selection_curr_id = -1;
                    needs_to_update = true;
                }

                if (needs_to_be_disabled)
                    style::endDisabled();

                ImGui::SameLine();
                ImGui::ProgressBar(progress);
            }

            // Trigger actual background processing as needed
            if (needs_to_update)
            {
                if (!presetSet)
                    resetPreset(); // Reset the preset menu if anything was changed here
                asyncProcess();
                needs_to_update = false;
            }

            // The image controls
            img_handler->drawMenu();
            if (img_handler->wasMenuTriggered) // If image got anything changed, reset too
                resetPreset();

            // Advanced controls
            if (enabled_advanced_menus)
            {
                if (ImGui::CollapsingHeader("Advanced"))
                {
                    if (ImGui::BeginTabBar("###imageproducttuning", ImGuiTabBarFlags_FittingPolicyScroll))
                    {
                        if (product->has_proj_cfg() && ImGui::BeginTabItem("Proj"))
                        {
                            widgets::JSONTreeEditor(product->contents["projection_cfg"], "##projcfgeditor");
                            ImGui::EndTabItem();
                        }

                        for (auto &ch : product->images)
                        {
                            std::string id = "Channel " + ch.channel_name;
                            if (ImGui::BeginTabItem(id.c_str()))
                            {
                                ch.ch_transform.render();
                                ImGui::EndTabItem();
                            }
                        }
                        ImGui::EndTabBar();
                    }
                }
            }
        }

        void ImageProductHandler::initOverlayConverters()
        {
            channels_calibration_units_and_converters.clear();
            if (channel_selection_curr_id == -1 && images_can_be_calibrated)
                return;

            // Just setup all possible converters and unit infos
            for (auto &cc : calibration::getAvailableConversions(product->images[channel_selection_curr_id].calibration_type))
            {
                auto conv = std::make_shared<calibration::UnitConverter>(product, product->images[channel_selection_curr_id].channel_name);
                conv->set_conversion(product->images[channel_selection_curr_id].calibration_type, cc);
                channels_calibration_units_and_converters.push_back({calibration::getUnitInfo(cc), conv});
            }
        }

        void ImageProductHandler::setConfig(nlohmann::json p)
        {
            if (p.contains("expression"))
            {
                expression = p["expression"];
                channel_selection_curr_id = -1;
            }
            else if (p.contains("channel"))
            {
                for (size_t i = 0; i < product->images.size(); i++)
                    if (product->images[i].channel_name == p["channel"].get<std::string>())
                        channel_selection_curr_id = i;
            }

            channel_calibrated = getValueOrDefault(p["channel_calibrated"], false);

            // Also handle automatic conversions!
            if (p.contains("calibration_ranges"))
            {
                auto &r = p["calibration_ranges"];
                for (size_t i = 0; i < product->images.size(); i++)
                {
                    auto &name = product->images[i].channel_name;
                    if (r.contains(name) && r[name].contains("min") && r[name].contains("max"))
                    {
                        for (auto u : channels_calibrated_ranges[i])
                        {
                            std::string curr_unit = u.first;
                            std::string source_unit = r[name].contains("unit") ? r[name]["unit"].get<std::string>() : product->images[i].calibration_type;
                            double min = r[name]["min"];
                            double max = r[name]["max"];

                            if (source_unit != curr_unit)
                            {
                                calibration::UnitConverter conv(product, name);
                                conv.set_conversion(source_unit, curr_unit);
                                if (!conv.convert_range(min, max))
                                {
                                    min = 0;
                                    max = 100;
                                }
                            }

                            channels_calibrated_ranges[i][u.first].min = min;
                            channels_calibrated_ranges[i][u.first].max = max;
                        }
                    }
                }
            }

            if (p.contains("image"))
                img_handler->setConfig(p["image"]);

            // TODOREWORK Maybe make it changed when updated NOT because of preset?
            if (p.contains("name"))
                product_internal_name = p["name"].get<std::string>();
            else
                product_internal_name = "Image";
            img_handler->setName(generateFileName());
        }

        nlohmann::json ImageProductHandler::getConfig()
        {
            nlohmann::json p;

            if (channel_selection_curr_id == -1)
                p["expression"] = expression;
            else
                p["channel"] = product->images[channel_selection_curr_id].channel_name;

            p["channel_calibrated"] = channel_calibrated;

            for (int i = 0; i < product->images.size(); i++)
            {
                if (channels_calibrated_ranges.count(i) && channels_calibrated_ranges[i].size() > 0)
                {
                    auto &r = p["calibration_ranges"];
                    auto &name = product->images[i].channel_name;
                    r[name]["min"] = channels_calibrated_ranges[i][product->images[i].calibration_type].min;
                    r[name]["max"] = channels_calibrated_ranges[i][product->images[i].calibration_type].max;
                }
            }

            p["image"] = img_handler->getConfig();

            return p;
        }

        void ImageProductHandler::do_process()
        {
            try
            {

                if (channel_selection_curr_id == -1)
                { // Expression case
                    auto img = products::generate_expression_product_composite(product, expression, &progress);
                    img_handler->setImage(img);
                }
                else
                {
                    image::Image img;
                    if (channel_calibrated)
                    { // Calibrated case
                        std::string &unit = channels_calibrated_curr_unit;
                        img = products::generate_calibrated_product_channel(product, product->images[channel_selection_curr_id].channel_name,
                                                                            channels_calibrated_ranges[channel_selection_curr_id][unit].min,
                                                                            channels_calibrated_ranges[channel_selection_curr_id][unit].max, unit, &progress);
                    }
                    else
                    { // Normal case
                        img = product->images[channel_selection_curr_id].image;
                        image::set_metadata_proj_cfg(img, product->get_proj_cfg(product->images[channel_selection_curr_id].abs_index));
                    }

                    img_handler->setImage(img);
                }
            }
            catch (std::exception &e)
            {
                logger->error("Could not process image! %s", e.what());
            }
        }

        void ImageProductHandler::saveResult(std::string directory) { img_handler->saveResult(directory); }

        void ImageProductHandler::drawMenuBar()
        {
            img_handler->drawMenuBar();
            if (ImGui::MenuItem("Image To Handler"))
            {
                std::shared_ptr<ImageHandler> a = std::make_shared<ImageHandler>();
                a->setConfig(img_handler->getConfig());
                a->setImage(img_handler->getImage(false));
                a->setName(img_handler->getName());
                addSubHandler(a);
            }
            if (ImGui::MenuItem("Advanced Mode", NULL, enabled_advanced_menus))
                enabled_advanced_menus = !enabled_advanced_menus;
        }

        void ImageProductHandler::drawContents(ImVec2 win_size) { img_handler->drawContents(win_size); }
    } // namespace handlers
} // namespace satdump
