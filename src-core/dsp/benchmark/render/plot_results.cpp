#include "plot_results.h"
#include "dsp/benchmark/render/imgui_sw.hpp"
#include "image/text.h"
#include "imgui/imgui.h"
#include "imgui/imgui_flags.h"
#include "imgui/implot/implot.h"

namespace satdump
{
    namespace ndsp
    {
        image::Image renderResults(std::vector<DSPBenchResult> results)
        {
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO &io = ImGui::GetIO();
            (void)io;
            io.IniFilename = NULL;

            int width_pixels = 1024, height_pixels = 1024;

            std::vector<uint32_t> pixel_buffer(width_pixels * height_pixels, 0);

            imgui_sw::bind_imgui_painting();

            ImPlot::CreateContext();

            imgui_sw::SwOptions sw_options;

            for (int i = 0; i < 10; i++)
            {
                io.DisplaySize = ImVec2((float)width_pixels, (float)height_pixels);
                ImGui::NewFrame();

                ImGui::SetNextWindowPos({0, 0});
                ImGui::SetNextWindowSize({(float)width_pixels, (float)height_pixels});
                ImGui::Begin("PlotWindow", 0, NOWINDOW_FLAGS | ImGuiWindowFlags_NoDecoration);

                ImPlot::BeginPlot("Results", ImVec2(width_pixels - 18, height_pixels - 18));

                ImPlot::SetupAxisLimits(ImAxis_X1, 0, results.size() + 1);

                int iii = 1;
                for (auto &result : results)
                {
                    ImPlot::PlotBars(result.id.c_str(), &result.throughput, 1, 0.67, iii++);
                }

                ImPlot::EndPlot();

                ImGui::End();

                ImGui::Render();

                std::fill_n(pixel_buffer.data(), pixel_buffer.size(), 0);
                paint_imgui(pixel_buffer.data(), width_pixels, height_pixels, sw_options);
            }

            image::Image img(8, width_pixels, height_pixels, 4);

            for (int i = 0; i < width_pixels * height_pixels; i++)
            {
                img.set(3, i, 0xFF); // (pixel_buffer[i] >> 24) & 0xFF);
                img.set(2, i, (pixel_buffer[i] >> 16) & 0xFF);
                img.set(1, i, (pixel_buffer[i] >> 8) & 0xFF);
                img.set(0, i, (pixel_buffer[i] >> 0) & 0xFF);
            }

            return img;
        }
    } // namespace ndsp
} // namespace satdump