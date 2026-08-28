#include "plot.h"
#include "dsp/benchmark/render/imgui_sw.hpp"
#include "imgui/imgui.h"
#include "imgui/imgui_flags.h"
#include "imgui/implot/implot.h"
#include "utils/time.h"
#include <cmath>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>
#include <algorithm>

namespace elektro_arktika
{
    namespace ggak
    {
        int DateFormatter2(double value, char *buff, int size, void *data)
        {
            std::string time;

            {
                double timestamp = value;

                if (timestamp < 0)
                    timestamp = 0;

                time_t tttime = timestamp;
                std::tm *timeReadable = gmtime(&tttime);
                std::stringstream timestamp_string;
                std::string timezone_string = "";

                timestamp_string << std::setfill('0') //
                                 /*<< timeReadable->tm_year + 1900 << "/"             //
                                 << std::setw(2) << timeReadable->tm_mon + 1 << "/" //
                                 << std::setw(2) << timeReadable->tm_mday << " "    //*/
                                 << std::setw(2) << timeReadable->tm_hour << ":" //
                                 << std::setw(2) << timeReadable->tm_min;        //
                                                                                 // << std::setw(2) << timeReadable->tm_sec         //
                                                                                 //<< timezone_string;

                time = timestamp_string.str();
            }

            time.resize(size, '\0');

            memcpy(buff, time.data(), time.size());

            return time.size();
        }

        image::Image plotData(std::string title, std::vector<double> time, std::vector<double> data, double time_start, double time_stop)
        {
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO &io = ImGui::GetIO();
            (void)io;
            io.IniFilename = NULL;

            int width_pixels = 512, height_pixels = 256;

            std::vector<uint32_t> pixel_buffer(width_pixels * height_pixels, 0);

            imgui_sw::bind_imgui_painting();

            ImPlot::CreateContext();

            imgui_sw::SwOptions sw_options;

            ImGui::PushStyleColor(ImGuiCol_FrameBg, (ImU32)ImColor(255, 255, 255));
            ImGui::PushStyleColor(ImGuiCol_Text, (ImU32)ImColor(0, 0, 0));

            ImPlot::PushStyleColor(ImPlotCol_PlotBg, (ImU32)ImColor(255, 255, 255));

            title += "\n";
            title += satdump::timestamp_to_string(time_start) + " to " + satdump::timestamp_to_string(time_stop);

            for (int i = 0; i < 10; i++)
            {
                io.DisplaySize = ImVec2((float)width_pixels, (float)height_pixels);
                ImGui::NewFrame();

                ImGui::SetNextWindowPos({0, 0});
                ImGui::SetNextWindowSize({(float)width_pixels, (float)height_pixels});
                ImGui::Begin("PlotWindow", 0, NOWINDOW_FLAGS | ImGuiWindowFlags_NoDecoration);

                ImPlot::BeginPlot(title.c_str(), ImVec2(width_pixels - 18, height_pixels - 18), ImPlotFlags_NoLegend);

                ImPlot::SetupAxisLimits(ImAxis_X1, time_start, time_stop);

                ImPlot::SetupAxisFormat(ImAxis_X1, DateFormatter2);
                std::vector<double> ticks;
                for (time_t t = (time_t(time_start) / 3600) * 3600; t < time_stop; t += 3600 * 6)
                    ticks.push_back(t);
                ImPlot::SetupAxisTicks(ImAxis_X1, ticks.data(), ticks.size());

                // If time offset is too large, set to NAN to avoid stray lines
                double last_time = 0;
                for (auto &dat : time)
                {
                    if (last_time != 0 && (dat - last_time) > 120)
                        dat = NAN;

                    last_time = dat;
                }

                ImPlot::PlotLine("Data", time.data(), data.data(), std::min<int>(time.size(), data.size()));

                ImPlot::EndPlot();

                ImGui::End();

                ImGui::Render();

                std::fill_n(pixel_buffer.data(), pixel_buffer.size(), 0xFFFFFFFF);
                paint_imgui(pixel_buffer.data(), width_pixels, height_pixels, sw_options);
            }

            ImGui::PopStyleColor(2);
            ImPlot::PopStyleColor();

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

        std::vector<image::Image> plotSKLData(std::string satellite, double time_start, double time_stop, std::vector<SKLRecord> skl_data)
        {
            std::vector<image::Image> all_skl_images;
            for (int i = 0; i < 12; i++)
            {
                std::vector<double> time, data;

                for (auto &r : skl_data)
                {
                    time.push_back(r.time);
                    data.push_back(r.dat[i]);
                }

                std::string labels[12] = {
                    "Electrons > 0.15 and protons > 1.5 MeV",
                    " Electrons > 3 and protons > 7 MeV",
                    "Electrons 3-6 MeV",
                    "Electrons 6-9 MeV",
                    "Electrons 8-15 MeV",
                    "Protons 9-20 MeV",
                    "Protons 20-40 MeV",
                    "Protons 40-110 MeV",
                    "Protons 110-300 MeV",
                    "Electrons > 0.6 and protons 1.6-200 MeV",
                    "Electrons > 1.3 and protons 1.6-200 MeV",
                    "Protons > 100 MeV",
                };

                auto img = plotData(satellite + " SKL - " + labels[i], time, data, time_start, time_stop);

                all_skl_images.push_back(img);
            }

            return all_skl_images;
        }
    } // namespace ggak
} // namespace elektro_arktika
