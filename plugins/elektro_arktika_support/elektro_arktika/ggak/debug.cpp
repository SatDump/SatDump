#include "debug.h"
#include "dsp/benchmark/render/imgui_sw.hpp"
#include "imgui/imgui.h"
#include "imgui/imgui_flags.h"
#include "imgui/implot/implot.h"
#include "utils/string.h"
#include "utils/time.h"
#include <cstring>
#include <fstream>

namespace elektro_arktika
{
    namespace ggak
    {
        std::vector<SKLRecord> debug_parseSKLCSV(std::string csv)
        {
            std::ifstream din(csv);

            std::vector<SKLRecord> dat;

            std::string line;
            while (std::getline(din, line))
            {
                if (line.size() <= 0)
                    continue;
                if (line[0] == '#')
                    continue;

                //   printf("%s\n", line.c_str());

                auto parts = satdump::splitString(line, ',');

                if (parts.size() == 13)
                {
                    SKLRecord rec;

                    tm timeS;
                    memset(&timeS, 0, sizeof(tm));
                    if (sscanf(parts[0].c_str(), "%4d-%2d-%2d %2d:%2d:%2d", &timeS.tm_year, &timeS.tm_mon, &timeS.tm_mday, &timeS.tm_hour, &timeS.tm_min, &timeS.tm_sec) == 6)
                    {
                        timeS.tm_year -= 1900;
                        timeS.tm_mon -= 1;
                        rec.time = timegm(&timeS);
                    }

                    bool anyisbelow0 = false;

                    for (int i = 0; i < 12; i++)
                    {
                        rec.dat[i] = std::stod(parts[1 + i]);
                        if (rec.dat[i] < 0)
                            anyisbelow0 = true;

                        // logger->info(" %d %lf %d", i + 1, rec.dat[i], int(anyisbelow0));
                    }

                    //  if (!anyisbelow0)
                    dat.push_back(rec);
                }
            }

            return dat;
        }

        namespace
        {
            int DateFormatter(double value, char *buff, int size, void *data)
            {
                std::string time = satdump::timestamp_to_string(value);
                time.resize(size, '\0');

                memcpy(buff, time.data(), time.size());

                return time.size();
            }
        } // namespace

        image::Image debug_plotSKLSingle(std::vector<SKLRecord> results1, int ch, bool off, double time_start, double time_stop)
        {
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO &io = ImGui::GetIO();
            (void)io;
            io.IniFilename = NULL;

            int width_pixels = 2048, height_pixels = 1024;

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

                ImPlot::SetupAxisLimits(ImAxis_X1, time_start, time_stop);
                ImPlot::SetupAxisFormat(ImAxis_X1, DateFormatter);
                std::vector<double> ticks;
                for (time_t t = (time_t(time_start) / 3600) * 3600; t < time_stop; t += 3600 * 2)
                    ticks.push_back(t);
                ImPlot::SetupAxisTicks(ImAxis_X1, ticks.data(), ticks.size());

                {

                    std::vector<double> times;
                    std::vector<double> data;

                    double min = DBL_MAX;
                    double max = DBL_MIN;
                    for (auto &rec : results1)
                    {
                        if (rec.time < time_start || rec.time > time_stop)
                            continue;
                        // logger->critical("%s %f", satdump::timestamp_to_string(rec.time).c_str(), rec.dat[0]);
                        times.push_back(rec.time);
                        data.push_back(rec.dat[ch]);

                        if (min > rec.dat[ch])
                            min = rec.dat[ch];
                        if (max < rec.dat[ch])
                            max = rec.dat[ch];
                    }

                    // if (i == 0)
                    //     logger->trace("%d %d %f %f", ch, i, min, max);

                    ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, max, ImPlotCond_Always);

                    ImPlot::PlotLine(off ? "SKL Off" : "SKL GGAK", times.data(), data.data(), times.size());
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

        image::Image debug_plotSKL(std::vector<SKLRecord> results1, std::vector<SKLRecord> results2, int ch, double time_start, double time_stop)
        {
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO &io = ImGui::GetIO();
            (void)io;
            io.IniFilename = NULL;

            int width_pixels = 2048, height_pixels = 1024;

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

                ImPlot::SetupAxisLimits(ImAxis_X1, time_start, time_stop);
                ImPlot::SetupAxisFormat(ImAxis_X1, DateFormatter);
                std::vector<double> ticks;
                for (time_t t = (time_t(time_start) / 3600) * 3600; t < time_stop; t += 3600 * 2)
                    ticks.push_back(t);
                ImPlot::SetupAxisTicks(ImAxis_X1, ticks.data(), ticks.size());

                {

                    std::vector<double> times;
                    std::vector<double> data;

                    double min = DBL_MAX;
                    double max = DBL_MIN;
                    for (auto &rec : results1)
                    {
                        if (rec.time < time_start || rec.time > time_stop)
                            continue;
                        // logger->critical("%s %f", satdump::timestamp_to_string(rec.time).c_str(), rec.dat[0]);
                        times.push_back(rec.time);
                        data.push_back(rec.dat[ch]);

                        if (min > rec.dat[ch])
                            min = rec.dat[ch];
                        if (max < rec.dat[ch])
                            max = rec.dat[ch];
                    }

                    ImPlot::SetupAxisLimits(ImAxis_Y1, 0, max);

                    ImPlot::PlotLine("SKL Off", times.data(), data.data(), times.size());
                }

                {
                    std::vector<double> times;
                    std::vector<double> data;

                    double min = DBL_MAX;
                    double max = DBL_MIN;
                    for (auto &rec : results2)
                    {
                        if (rec.time < time_start || rec.time > time_stop)
                            continue;
                        // logger->critical("%s %f", satdump::timestamp_to_string(rec.time).c_str(), rec.dat[0]);
                        times.push_back(rec.time);
                        data.push_back(rec.dat[ch]);

                        if (min > rec.dat[ch])
                            min = rec.dat[ch];
                        if (max < rec.dat[ch])
                            max = rec.dat[ch];
                    }

                    /*for (auto &rec : data)
                    {
                        double val = (rec - min) / (max - min);
                        double val2 = (val * (prev_max - prev_min)) + prev_min;
                        rec = val2;
                    }*/

                    // ImPlot::SetupAxisLimits(ImAxis_Y1, 0, max);

                    ImPlot::PlotLine("SKL GGAK", times.data(), data.data(), times.size());
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
    } // namespace ggak
} // namespace elektro_arktika