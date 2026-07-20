/**********************************************************************
 * This file is used for testing random stuff without running the
 * whole of SatDump, which comes in handy for debugging individual
 * elements before putting them all together in modules...
 *
 * If you are an user, ignore this file which will not be built by
 * default, and if you're a developper in need of doing stuff here...
 * Go ahead!
 *
 * Don't judge the code you might see in there! :)
 **********************************************************************/

#include "common/codings/deframing/bpsk_ccsds_deframer.h"
#include "common/simple_deframer.h"
#include "core/style.h"
#include "dsp/benchmark/render/imgui_sw.hpp"
#include "image/image.h"
#include "image/io.h"
#include "image/processing.h"
#include "image/text.h"
#include "imgui/imgui.h"
#include "imgui/imgui_flags.h"
#include "imgui/implot/implot.h"
#include "logger.h"
#include "utils/string.h"
#include "utils/time.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <ostream>
#include <string>
#include <vector>

image::Image renderResults(std::vector<double> xdat, std::vector<double> ydat)
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

    for (int i = 0; i < 10; i++)
    {
        io.DisplaySize = ImVec2((float)width_pixels, (float)height_pixels);
        ImGui::NewFrame();

        ImGui::SetNextWindowPos({0, 0});
        ImGui::SetNextWindowSize({(float)width_pixels, (float)height_pixels});
        ImGui::Begin("PlotWindow", 0, NOWINDOW_FLAGS | ImGuiWindowFlags_NoDecoration);

        ImPlot::BeginPlot("ELEKTRO-L2 SKL Ch1", ImVec2(width_pixels - 18, height_pixels - 18), ImPlotFlags_NoLegend);

        ImPlot::SetupAxisLimits(ImAxis_X1, 0, xdat.size() + 1);

        ImPlot::PlotLine("SKL Ch 1", xdat.data(), ydat.data(), std::min(xdat.size(), ydat.size()));

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

int main(int argc, char *argv[])
{
    initLogger();

    {
        std::vector<double> xdat, ydat;

        for (int i = 0; i < 500; i++)
        {
            xdat.push_back(i);
            ydat.push_back(sin(i / 10.));
        }

        auto img = renderResults(xdat, ydat);

        img.to_rgb();

        image::linear_invert(img);

        image::save_img(img, "/tmp/test.jpg");
    }

    return 0;

    std::vector<std::pair<double, double>> data;

    for (int i = 0; i < 500; i++)
        data.push_back({i, sin(i / 10.)});

    image::Image img(16, 512, 256, 3);
    img.fill_color({1, 1, 1});

    image::TextDrawer tdra;
    tdra.init_font("resources/fonts/Perfect-DOS-VGA-437-Win.ttf");

    img.draw_line(0, 256 - 12, 512, 256 - 12, {0, 0, 0});

    std::string text = "ELEKTRO-L2 GGAK SKL Ch1 (1 KeV Protons)";
    int l = tdra.draw_text(img, -100, -100, {0, 0, 0}, 10, text);
    tdra.draw_text(img, 256 - (l / 2), 256 - 10, {0, 0, 0}, 10, text);

    image::save_img(img, "/tmp/test.jpg");

    return 0;

    uint8_t frm[1024];
    std::ifstream din(argv[1]);
    std::ofstream dou(argv[2]);

    // auto deframer = std::make_unique<deframing::BPSK_CCSDS_Deframer>(8272, 0x352EF853);

    def::SimpleDeframer def(0x352EF853, 32, 8282, 0, false, false);

    while (!din.eof())
    {
        din.read((char *)frm, 1024);

        auto frs = def.work(frm + 4, 7136 / 8);
        for (auto &f : frs)
        {
            dou.write((char *)f.data(), 1034);
        }
    }
}