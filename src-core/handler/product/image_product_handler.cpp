#include "image_product_handler.h"
#include "nlohmann/json_utils.h"

#include "core/style.h"
#include "imgui/imgui_stdlib.h"
#include "logger.h"

#include "common/dsp_source_sink/format_notated.h" // TODOREWORK
#include "imgui/imgui_stdlib.h"
#include "products2/image/product_expression.h"

#include "common/widgets/json_editor.h"
#include "products2/image/calibration_units.h" // TODOREWORK

namespace satdump
{
    namespace viewer
    {
        ImageProductHandler::ImageProductHandler(std::shared_ptr<products::Product> p, bool dataset_mode) : ProductHandler(p, dataset_mode)
        {
            handler_tree_icon = "\uf71e";

            product = (products::ImageProduct *)ProductHandler::product.get();
            for (auto &img : product->images)
                channel_selection_box_str += "Channel " + img.channel_name + '\0';

            // TODOREWORK
            // if (product->has_calibration())
            img_calibrator = products::get_calibrator_from_product(product);
            if (img_calibrator)
                images_can_be_calibrated = true;

            img_handler = std::make_shared<ImageHandler>();

            img_handler->additionalMouseCallback = [this](int x, int y)
            {
                if (is_processing)
                    return;

                if (channel_selection_curr_id != -1)
                    ImGui::Text("Count : %d", product->get_raw_channel_val(channel_selection_curr_id, x, y));

                if (!channel_calibrated && img_calibrator && channel_selection_curr_id != -1)
                {
                    double val = img_calibrator->compute(channel_selection_curr_id, x, y);
                    ImGui::Text("Unit : %f %s", val, product->images[channel_selection_curr_id].calibration_type.c_str());
                }
            };

            // TODOREWORK Calib range init
            channel_calibrated_range_min.resize(product->images.size(), 0);
            channel_calibrated_range_max.resize(product->images.size(), 100);
            if (images_can_be_calibrated)
            {
                if (channel_selection_curr_id != -1)
                {
                    channel_calibrated_output_units = calibration::getAvailableConversions(product->images[channel_selection_curr_id].calibration_type);
                    channel_calibrated_combo_str.clear();
                    for (auto &u : channel_calibrated_output_units)
                        channel_calibrated_combo_str += calibration::getUnitInfo(u).name + '\0';
                }
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

                if (ImGui::Combo("##imageproductchannelcombo", &channel_selection_curr_id, channel_selection_box_str.c_str()))
                {
                    if (channel_selection_curr_id != -1)
                    { // TODOREWORK dedup
                        channel_calibrated_output_units = calibration::getAvailableConversions(product->images[channel_selection_curr_id].calibration_type);
                        channel_calibrated_combo_str.clear();
                        for (auto &u : channel_calibrated_output_units)
                            channel_calibrated_combo_str += calibration::getUnitInfo(u).name + '\0';
                    }

                    needs_to_update = true;
                }

                if (channel_selection_curr_id != -1)
                {
                    auto &ch = product->images[channel_selection_curr_id];

                    if (ch.wavenumber != -1)
                    {
                        auto freq = product->get_channel_frequency(ch.abs_index);
                        ImGui::SameLine();
                        ImGui::Text("(%s)", format_notated(SPEED_OF_LIGHT_M_S / freq, "m", 2).c_str());
                        ImGui::Text("%.3f cm\u207b\u00b9 / %s", ch.wavenumber, format_notated(freq, "Hz", 2).c_str());
                    }
                }

                if (channel_selection_curr_id != -1 && images_can_be_calibrated)
                {
                    needs_to_update |= ImGui::Checkbox("Calibrate", &channel_calibrated);
                    ImGui::SetNextItemWidth(150 * ui_scale);
                    ImGui::InputDouble("##rangemin", &channel_calibrated_range_min[channel_selection_curr_id]);
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(150 * ui_scale);
                    ImGui::InputDouble("##rangemax", &channel_calibrated_range_max[channel_selection_curr_id]);
                    needs_to_update |= ImGui::Combo("Unit##calibunit", &channel_calibrated_combo_curr_id, channel_calibrated_combo_str.c_str());
                    needs_to_update |= ImGui::Button("Update###updatecalib");
                    ImGui::SameLine();
                    if (ImGui::Button("Add To Equ###calibaddtoexpression")) // TODOREWORK?
                        expression = "cch" + product->images[channel_selection_curr_id].channel_name + "=(" + product->images[channel_selection_curr_id].channel_name + ", " +
                                     channel_calibrated_output_units[channel_calibrated_combo_curr_id] + ", " + std::to_string(channel_calibrated_range_min[channel_selection_curr_id]) + ", " +
                                     std::to_string(channel_calibrated_range_max[channel_selection_curr_id]) + ");\n" + expression;
                }

                if (needs_to_be_disabled)
                    style::endDisabled();
            }

            needs_to_update |= renderPresetMenu(); // TODOREWORK move in top drawMenu?

            if (ImGui::CollapsingHeader("Expression", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (needs_to_be_disabled)
                    style::beginDisabled();

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

            /// TODOREWORK UPDATE
            if (needs_to_update)
            {
                asyncProcess();
                needs_to_update = false;
            }

            img_handler->drawMenu();

            if (ImGui::CollapsingHeader("ImageProduct Advanced"))
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

        void ImageProductHandler::setConfig(nlohmann::json p)
        {
            if (p.contains("expression"))
            {
                expression = p["expression"];
                channel_selection_curr_id = -1;
            }
            else if (p.contains("channel"))
            {
                for (int i = 0; i < product->images.size(); i++)
                    if (product->images[i].channel_name == p["channel"].get<std::string>())
                        channel_selection_curr_id = i;
            }

            channel_calibrated = getValueOrDefault(p["channel_calibrated"], false);

            if (p.contains("calibration_ranges"))
            {
                auto &r = p["calibration_ranges"];
                for (int i = 0; i < product->images.size(); i++)
                {
                    auto &name = product->images[i].channel_name;
                    if (r.contains(name) && r[name].contains("min") && r[name].contains("max"))
                    {
                        channel_calibrated_range_min[i] = r[name]["min"];
                        channel_calibrated_range_max[i] = r[name]["max"];
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
            img_handler->image_name = generateFileName();
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
                auto &r = p["calibration_ranges"];
                auto &name = product->images[i].channel_name;
                r[name]["min"] = channel_calibrated_range_min[i];
                r[name]["max"] = channel_calibrated_range_max[i];
            }

            p["image"] = img_handler->getConfig();

            return p;
        }

        void ImageProductHandler::do_process()
        {
            try
            {
                if (channel_selection_curr_id == -1)
                {
                    auto img = products::generate_expression_product_composite(product, expression, &progress);
                    img_handler->updateImage(img);
                }
                else
                {
                    image::Image img;
                    if (channel_calibrated)
                        img = products::generate_calibrated_product_channel(product, product->images[channel_selection_curr_id].channel_name, channel_calibrated_range_min[channel_selection_curr_id],
                                                                            channel_calibrated_range_max[channel_selection_curr_id], channel_calibrated_output_units[channel_calibrated_combo_curr_id],
                                                                            &progress);
                    else
                    {
                        img = product->images[channel_selection_curr_id].image;
                        image::set_metadata_proj_cfg(img, product->get_proj_cfg(product->images[channel_selection_curr_id].abs_index));
                    }

                    img_handler->updateImage(img);
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
                a->updateImage(img_handler->image);
                a->image_name = img_handler->image_name;
                addSubHandler(a);
            }
        }

        void ImageProductHandler::drawContents(ImVec2 win_size) { img_handler->drawContents(win_size); }
    } // namespace viewer
} // namespace satdump
