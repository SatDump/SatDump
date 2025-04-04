#include "lut_generator.h"
#include "imgui/implot/implot.h"
#include "common/image/io.h"
#include "logger.h"

namespace satdump
{
    namespace viewer
    {
        LutGeneratorHandler::LutGeneratorHandler()
        {
            lut.resize(100, {255, 255, 255, 255});
            for (int i = 0; i < 100; i++)
                lut[i].r = lut[i].g = lut[i].b = (i / 100.0) * 255;
        }

        LutGeneratorHandler::~LutGeneratorHandler()
        {
        }

        void LutGeneratorHandler::drawMenu()
        {
        }

        void LutGeneratorHandler::drawContents(ImVec2 win_size)
        {
            ImGui::Text("Size : %d\n", lut.size());
            if (ImGui::Button("+"))
                lut.push_back(current_col);
            if (ImGui::Button("-"))
                lut.erase(lut.end() - 1);

            ImPlot::BeginPlot("LutPlot");
            // ImPlot::SetupAxes("", "", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
            ImPlot::SetupAxisLimits(ImAxis_X1, 0, lut.size(), ImPlotCond_Always);
            ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 1, ImPlotCond_Always);
            for (int i = 0; i < lut.size(); i++)
            {
                auto &p = lut[i];
                ImPlot::GetPlotDrawList()->AddQuadFilled(
                    ImPlot::PlotToPixels({i, 0}),
                    ImPlot::PlotToPixels({i + 1, 0}),
                    ImPlot::PlotToPixels({i + 1, 1}),
                    ImPlot::PlotToPixels({i, 1}),
                    IM_COL32(p.r, p.g, p.b, p.a));
            }

            auto p = ImPlot::GetPlotMousePos();
            if (p.y >= 0 && p.y <= 1 && p.x >= 0 && p.x < lut.size() && ImGui::IsItemFocused())
            {
                ImPlot::GetPlotDrawList()->AddLine(ImPlot::PlotToPixels({p.x, 0}),
                                                   ImPlot::PlotToPixels({p.x, 1}),
                                                   ImColor(0, 0, 0), 1);
                if (mouse_mode == 0)
                {
                    if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
                        lut[floor(p.x)] = current_col;
                }
                else if (mouse_mode == 1)
                {
                    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                    {
                        interpolate_start_position = floor(p.x);
                        interpolate_start_color = lut[interpolate_start_position];
                    }

                    if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
                    {
                        int interpolate_stop_position = floor(p.x);
                        auto interpolate_stop_color = lut[interpolate_stop_position];

                        int start = interpolate_start_position < interpolate_stop_position ? interpolate_start_position : interpolate_stop_position;
                        int stop = interpolate_start_position < interpolate_stop_position ? interpolate_stop_position : interpolate_start_position;

                        auto startc = interpolate_start_position < interpolate_stop_position ? interpolate_start_color : interpolate_stop_color;
                        auto stopc = interpolate_start_position < interpolate_stop_position ? interpolate_stop_color : interpolate_start_color;

                        printf("Start %d Stop %d\n", start, stop);
                        for (int i = start + 1; i < stop; i++)
                        {
                            float p = float(i - start) / float(stop - start);
                            printf("%f\n", p);
                            ColorPX f;
                            f.r = startc.r * (1.0 - p) + stopc.r * p;
                            f.g = startc.g * (1.0 - p) + stopc.g * p;
                            f.b = startc.b * (1.0 - p) + stopc.b * p;
                            f.a = startc.a * (1.0 - p) + stopc.a * p;
                            lut[i] = f;
                        }
                    }
                }

                if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
                    should_regen_image = true;
            }

            ImPlot::EndPlot();

            if (ImGui::RadioButton("Fill", mouse_mode == 0))
                mouse_mode = 0;
            if (ImGui::RadioButton("Interpolate", mouse_mode == 1))
                mouse_mode = 1;

            float colors[4] = {current_col.r / 255.0,
                               current_col.g / 255.0,
                               current_col.b / 255.0,
                               current_col.a / 255.0};
            ImGui::ColorEdit4("Color", colors, ImGuiColorEditFlags_NoSidePreview);
            current_col.r = colors[0] * 255.0;
            current_col.g = colors[1] * 255.0;
            current_col.b = colors[2] * 255.0;
            current_col.a = colors[3] * 255.0;

            ImGui::SameLine();

            if (ImGui::Button("Pick"))
                current_col = getColorFromScreen();

            image_select.draw();
            preview_img.draw({500, 500});

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
        }

        LutGeneratorHandler::ColorPX LutGeneratorHandler::getColorFromScreen()
        {
            FILE *grabcFile = popen("grabc", "r");

            if (!grabcFile)
                return {0, 0, 0, 0};

            char buffer[1024];
            char *line_p = fgets(buffer, sizeof(buffer), grabcFile);

            std::string hex(line_p + 1, line_p + 7);

            uint8_t r = std::stoul(hex.substr(0, 2), nullptr, 16);
            uint8_t g = std::stoul(hex.substr(2, 2), nullptr, 16);
            uint8_t b = std::stoul(hex.substr(4, 2), nullptr, 16);

            logger->trace("R %d G %d B %d", r, g, b);

            pclose(grabcFile);

            return {r, g, b, 255};
        }
    }
}