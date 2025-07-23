#include "lut_generator.h"
#include "core/resources.h"
#include "image/io.h"
#include "imgui/dialogs/pfd_utils.h"
#include "imgui/implot/implot.h"
#include "logger.h"
#include <cstdint>

namespace satdump
{
    namespace lutgen
    {
        LutGeneratorHandler::LutGeneratorHandler()
        {
            handler_tree_icon = u8"\uf043";

            lut.resize(100, {255, 255, 255, 255});
            for (int i = 0; i < 100; i++)
                lut[i].r = lut[i].g = lut[i].b = (i / 100.0) * 255;

            file_save_menu.getimg_callback = [this]()
            {
                auto img = std::make_shared<image::Image>();
                *img = generateLutImage();
                return img;
            };
        }

        LutGeneratorHandler::~LutGeneratorHandler() {}

        void LutGeneratorHandler::drawMenu()
        {
            if (ImGui::CollapsingHeader("LUT", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (ImGui::Button("--"))
                    for (int i = 0; i < 10; i++)
                        if (lut.size() > 1)
                            lut.erase(lut.end() - 1);
                ImGui::SameLine();
                if (ImGui::Button("-"))
                    if (lut.size() > 1)
                        lut.erase(lut.end() - 1);

                ImGui::SameLine();
                ImGui::Text("Size : %d\n", lut.size());
                ImGui::SameLine();

                if (ImGui::Button("+"))
                    lut.push_back(current_col);
                ImGui::SameLine();
                if (ImGui::Button("++"))
                    for (int i = 0; i < 10; i++)
                        lut.push_back(current_col);
                
                ImGui::Text("Mode:");
                ImGui::SameLine();
                if (ImGui::RadioButton("Fill", mouse_mode == 0))
                    mouse_mode = 0;
                ImGui::SameLine();
                if (ImGui::RadioButton("Interpolate", mouse_mode == 1))
                    mouse_mode = 1;

                float colors[4] = {current_col.r / 255.0f, current_col.g / 255.0f, current_col.b / 255.0f, current_col.a / 255.0f};
                ImGui::ColorEdit4("Color", colors, ImGuiColorEditFlags_NoInputs);
                current_col.r = colors[0] * 255.0f;
                current_col.g = colors[1] * 255.0f;
                current_col.b = colors[2] * 255.0f;
                current_col.a = colors[3] * 255.0f;

                ImGui::SameLine();

                if (ImGui::Button("Pick"))
                    current_col = getColorFromScreen();

                bool has_hist = history_vector.size();
                if (!has_hist)
                    style::beginDisabled();
                if (ImGui::Button("Undo"))
                    restoreHistory();
                if (!has_hist)
                    style::endDisabled();
                ImGui::SameLine();
                if (ImGui::Button("Invert"))
                    std::reverse(lut.begin(), lut.end());

                if (!use_range)
                    style::beginDisabled();
                ImGui::InputDouble("Range Min", &range_min);
                ImGui::InputDouble("Range Max", &range_max);
                if (range_max - range_min <= 0)
                    range_max = range_min + 1;
                if (!use_range)
                    style::endDisabled();
                ImGui::Checkbox("Use Range", &use_range);

                ImGui::InputInt("##replicate", &replicate_times);
                ImGui::SameLine();
                if (ImGui::Button("Replicate"))
                {
                    if (replicate_times >= 2)
                    {
                        auto lut2 = lut;
                        lut.clear();
                        for (int i = 0; i < replicate_times; i++)
                            lut.insert(lut.end(), lut2.begin(), lut2.end());
                    }
                }

                ImGui::InputInt("##interpolate", &interpolate_size);
                ImGui::SameLine();
                if (ImGui::Button("Interpolate##interpolatebutton"))
                {
                    auto img = generateLutImage();
                    img.resize_bilinear(interpolate_size, 1);
                    lut.clear();
                    for (int i = 0; i < interpolate_size; i++)
                        lut.push_back({(uint8_t)img.get(0, i, 0), (uint8_t)img.get(1, i, 0), (uint8_t)img.get(2, i, 0), (uint8_t)img.get(3, i, 0)});
                }
            }
            if (ImGui::CollapsingHeader("Preview Image", ImGuiTreeNodeFlags_DefaultOpen))
            {
                image_select.draw();
                if (ImGui::Button("Refresh Preview"))
                    should_regen_image = true;
            }
        }

        void LutGeneratorHandler::drawMenuBar()
        {
            file_save_menu.render("Save LUT", "lut", ".", "Save Image", true);
            file_open_menu.render("Load LUT", "Select Image", resources::getResourcePath("lut"), {{"All Files", "*"}});
        }

        void LutGeneratorHandler::drawContents(ImVec2 win_size)
        {
            if (!use_range)
            {
                range_min = 0;
                range_max = lut.size();
            }

            if (ImPlot::BeginPlot("LUT plot", {win_size.x - 10 * ui_scale, 200 * ui_scale}))
            {
                // ImPlot::SetupAxes("", "", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
                ImPlot::SetupAxisLimits(ImAxis_X1, range_min, range_max, ImPlotCond_Always);
                ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 1, ImPlotCond_Always);
                for (int i = 0; i < lut.size(); i++)
                {
                    auto &p = lut[i];
                    ImPlot::GetPlotDrawList()->AddQuadFilled(ImPlot::PlotToPixels({range_transform(i), 0}), ImPlot::PlotToPixels({range_transform(i + 1), 0}),
                                                             ImPlot::PlotToPixels({range_transform(i + 1), 1}), ImPlot::PlotToPixels({range_transform(i), 1}), IM_COL32(p.r, p.g, p.b, p.a));
                }

                ImPlotPoint p_i;
                ImPlotPoint p = p_i = ImPlot::GetPlotMousePos();
                p.x = range_transform_inv(p.x);
                if (p.y >= 0 && p.y <= 1 && p.x >= 0 && p.x < lut.size() && ImGui::IsItemFocused())
                {
                    ImPlot::GetPlotDrawList()->AddLine(ImPlot::PlotToPixels({p_i.x, 0}), ImPlot::PlotToPixels({p_i.x, 1}), ImColor(0, 0, 0), 1);

                    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                        saveHistory();

                    if (mouse_mode == 0)
                    {
                        if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
                        {
                            lut[floor(p.x)] = current_col;
                            lutaction_started = true;
                        }
                    }
                    else if (mouse_mode == 1)
                    {
                        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                        {
                            interpolate_start_position = floor(p.x);
                            interpolate_start_color = lut[interpolate_start_position];
                            lutaction_started = true;
                        }

                        if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
                        {
                            int interpolate_stop_position = floor(p.x);
                            auto interpolate_stop_color = lut[interpolate_stop_position];

                            int start = interpolate_start_position < interpolate_stop_position ? interpolate_start_position : interpolate_stop_position;
                            int stop = interpolate_start_position < interpolate_stop_position ? interpolate_stop_position : interpolate_start_position;

                            auto startc = interpolate_start_position < interpolate_stop_position ? interpolate_start_color : interpolate_stop_color;
                            auto stopc = interpolate_start_position < interpolate_stop_position ? interpolate_stop_color : interpolate_start_color;

                            //                        printf("Start %d Stop %d\n", start, stop);
                            for (int i = start + 1; i < stop; i++)
                            {
                                float p = float(i - start) / float(stop - start);
                                ColorPX f;
                                f.r = startc.r * (1.0 - p) + stopc.r * p;
                                f.g = startc.g * (1.0 - p) + stopc.g * p;
                                f.b = startc.b * (1.0 - p) + stopc.b * p;
                                f.a = startc.a * (1.0 - p) + stopc.a * p;
                                lut[i] = f;
                            }
                        }
                    }
                }

                ImPlot::EndPlot();
            }
            ImGui::Text("Preview Image");
            if (lutaction_started && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
                should_regen_image = true;
            if (ImGui::IsKeyDown(ImGuiKey_ModCtrl) && ImGui::IsKeyPressed(ImGuiKey_Z))
                restoreHistory();

            preview_img.draw({win_size.x, win_size.y - 200 * ui_scale});

            if (image_select.isValid() && should_regen_image)
            {
                image::Image img;
                image::load_img(img, image_select.getPath());

                image::Image limg(8, img.width(), img.height(), 4);
                for (size_t i = 0; i < img.width() * img.height(); i++)
                {
                    float val = img.getf(i);
                    int pos = val * (lut.size() - 1);
                    limg.set(0, i, lut[pos].r);
                    limg.set(1, i, lut[pos].g);
                    limg.set(2, i, lut[pos].b);
                    limg.set(3, i, lut[pos].a);
                }
                preview_img.update(limg);
                should_regen_image = false;
            }

            if (file_open_menu.update())
            {
                image::Image l;
                image::load_img(l, file_open_menu.getPath());
                if (l.size() == 0)
                {
                    logger->error("Invalid image for a LUT!");
                }
                else
                {
                    l = l.to8bits();
                    l.to_rgba();
                    lut.resize(l.width());
                    for (int i = 0; i < l.width(); i++)
                    {
                        lut[i].r = l.get(0, i, 0);
                        lut[i].g = l.get(1, i, 0);
                        lut[i].b = l.get(2, i, 0);
                        lut[i].a = l.get(3, i, 0);
                    }
                }
            }
        }

        image::Image LutGeneratorHandler::generateLutImage()
        {
            image::Image lutImg(8, lut.size(), 10, 4);
            for (size_t x = 0; x < lut.size(); x++)
            {
                for (size_t y = 0; y < lutImg.height(); y++)
                {
                    lutImg.set(0, x, y, lut[x].r);
                    lutImg.set(1, x, y, lut[x].g);
                    lutImg.set(2, x, y, lut[x].b);
                    lutImg.set(3, x, y, lut[x].a);
                }
            }
            return lutImg;
        }

        LutGeneratorHandler::ColorPX LutGeneratorHandler::getColorFromScreen()
        {
#ifndef _WIN32
            FILE *grabcFile = popen("grabc", "r");

            if (!grabcFile)
            {
                logger->critical("Please use Linux and install grabc! \ue712 (or remind us maybe we should support Windows too?)");
                return {0, 0, 0, 0};
            }

            char buffer[1024];
            char *line_p = fgets(buffer, sizeof(buffer), grabcFile);

            if (line_p == nullptr)
            {
                logger->critical("Please use Linux and install grabc! \ue712 (or remind us maybe we should support Windows too?)");
                return {0, 0, 0, 0};
            }

            std::string hex(line_p + 1, line_p + 7);

            uint8_t r = std::stoul(hex.substr(0, 2), nullptr, 16);
            uint8_t g = std::stoul(hex.substr(2, 2), nullptr, 16);
            uint8_t b = std::stoul(hex.substr(4, 2), nullptr, 16);

            logger->trace("R %d G %d B %d", r, g, b);

            pclose(grabcFile);

            return {r, g, b, 255};
#else
            return {0, 0, 0, 255};
#endif
        }
    } // namespace lutgen
} // namespace satdump