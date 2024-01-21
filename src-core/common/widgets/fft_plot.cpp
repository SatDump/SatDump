#include "fft_plot.h"
#include "imgui/imgui_internal.h"
#include <string>
#include "common/dsp_source_sink/format_notated.h"
#include "core/module.h"
#include "resources.h"

namespace widgets
{
    FFTPlot::FFTPlot(float *v, int size, float min, float max, float scale_res)
    {
        values = v;
        values_size = size;
        scale_min = min;
        scale_max = max;
        scale_resolution = scale_res;
    }

    void FFTPlot::draw(ImVec2 size)
    {
        work_mutex.lock();
        ImGuiWindow *window = ImGui::GetCurrentWindow();
        ImGuiContext &g = *GImGui;
        const ImGuiStyle &style = g.Style;
        const ImRect frame_bb(window->DC.CursorPos, {window->DC.CursorPos.x + size.x, window->DC.CursorPos.y + size.y});
        const ImRect inner_bb({frame_bb.Min.x, frame_bb.Min.y + style.FramePadding.y},
                              {frame_bb.Max.x - 1, frame_bb.Max.y - style.FramePadding.y});

        int res_w = size.x; // /*ImMin((int)size.x,*/ values_size /*)*/ - 1;
        // int item_count = values_size - 1;
        const float t_step = 1.0f / (float)res_w;
        const float inv_scale = (scale_min == scale_max) ? 0.0f : (1.0f / (scale_max - scale_min));
        float v0 = values[0];
        float t0 = 0.0f;
        ImVec2 tp0 = ImVec2(t0, 1.0f - ImSaturate((v0 - scale_min) * inv_scale));
        const ImU32 col_base = ImGui::GetColorU32(ImGuiCol_PlotLines);

        // Background
        ImGui::RenderFrame(frame_bb.Min, frame_bb.Max, ImGui::GetColorU32(ImGuiCol_FrameBg), true, style.FrameRounding);

        // Draw lines
        float vscale = ((scale_max - scale_min) / scale_resolution);
        float step = (frame_bb.Max.y - frame_bb.Min.y) / scale_resolution;
        float value = scale_min;
        const ImU32 color_scale = ImGui::GetColorU32(ImGuiCol_Text, 0.4);
        for (float i = frame_bb.Max.y - step; i >= frame_bb.Min.y; i -= step)
        {
            ImVec2 pos0 = {frame_bb.Min.x, i};
            ImVec2 pos1 = {frame_bb.Max.x, i};
            window->DrawList->AddLine(pos0, pos1, color_scale);
            value += vscale;
            window->DrawList->AddText({pos0.x, pos0.y + 2}, color_scale, std::string(std::to_string(int(value)) + " dB").c_str());
        }

        // Draw Freq Scale
        // const ImU32 fcolor_scale = ImGui::GetColorU32(ImGuiCol_Text, 1.0);
        if (enable_freq_scale && bandwidth != 0 && frequency != 0)
        {
            float y_level = inner_bb.Max.y - 8 * ui_scale;
            window->DrawList->AddLine({inner_bb.Min.x, y_level}, {inner_bb.Max.x, y_level}, col_base, 3 * ui_scale);
            for (int i = 1; i < 10; i++)
            {
                float x_line = inner_bb.Min.x + (i / 10.0) * res_w;
                double freq_here = frequency + ((i / 10.0) * bandwidth) - (bandwidth / 2);

                window->DrawList->AddLine({x_line, y_level}, {x_line, y_level - 10 * ui_scale}, col_base, 3);

                auto cstr = format_notated(freq_here);
                window->DrawList->AddText({x_line - ImGui::CalcTextSize(cstr.c_str()).x / 2, y_level - 30 * ui_scale},
                                          i == 5 ? 0xFF00FF00 : col_base,
                                          cstr.c_str());
                if (i == 5 && actual_sdr_freq != -1)
                {
                    auto cstr = "(" + format_notated(actual_sdr_freq) + ")";
                    window->DrawList->AddText({x_line - ImGui::CalcTextSize(cstr.c_str()).x / 2, y_level - 16 * ui_scale},
                                              i == 5 ? 0xFF0000FF : col_base,
                                              cstr.c_str());
                }
            }
        }

        // Draw VFOs
        if (bandwidth != 0 && frequency != 0 && vfo_freqs.size() > 0)
        {
            const ImU32 col_base_vfo = ImColor(255, 0, 0, 255);

            for (auto vfo : vfo_freqs)
            {
                double vfreq = vfo.second - frequency;
                float y_level = inner_bb.Max.y - 8 * ui_scale;
                float x_line = inner_bb.Min.x + (((bandwidth / 2) + vfreq) / bandwidth) * res_w;

                window->DrawList->AddLine({x_line, y_level}, {x_line, y_level - 10 * ui_scale}, col_base_vfo, 3);
                window->DrawList->AddLine({x_line, y_level - 10 * ui_scale}, {x_line - 5 * ui_scale, y_level - 20 * ui_scale}, col_base_vfo, 3);
                window->DrawList->AddLine({x_line, y_level - 10 * ui_scale}, {x_line + 5 * ui_scale, y_level - 20 * ui_scale}, col_base_vfo, 3);

                if (sqrtf(powf(ImGui::GetMousePos().x - x_line, 2) + powf(ImGui::GetMousePos().y - y_level, 2)) < 40 * ui_scale)
                    window->DrawList->AddText({x_line - ImGui::CalcTextSize(vfo.first.c_str()).x / 2, y_level - 45 * ui_scale},
                                              col_base_vfo,
                                              vfo.first.c_str());
            }
        }

        // Draw lines
        double fz = (double)values_size / (double)res_w;
        for (int n = 0; n < res_w; n++)
        {
            const float t1 = t0 + t_step;
            // const int v1_idx = (int)(t0 * item_count + 0.5f);

            float ffpos = n * fz;

            if (ffpos >= values_size)
                ffpos = values_size - 1;

            float finals = -INFINITY;
            for (float v = ffpos; v < ffpos + fz; v += 1)
                if (finals < values[(int)floor(v)])
                    finals = values[(int)floor(v)];

            const float v1 = finals; // values[v1_idx];
            const ImVec2 tp1 = ImVec2(t1, 1.0f - ImSaturate((v1 - scale_min) * inv_scale));

            ImVec2 pos0 = ImLerp(inner_bb.Min, inner_bb.Max, tp0);
            ImVec2 pos1 = ImLerp(inner_bb.Min, inner_bb.Max, tp1);

            window->DrawList->AddLine(pos0, pos1, col_base);

            t0 = t1;
            tp0 = tp1;
        }

        ImGui::Dummy(size);
        work_mutex.unlock();
    }

    image::Image<uint8_t> FFTPlot::drawImg(int size_x, int size_y)
    {
        work_mutex.lock();
        image::Image<uint8_t> img(size_x, size_y, 3);
        img.init_font(resources::getResourcePath("fonts/font.ttf"));

        int res_w = size_x;

        const float t_step = 1.0f / (float)res_w;
        const float inv_scale = (scale_min == scale_max) ? 0.0f : (1.0f / (scale_max - scale_min));
        float t0 = 0.0f;
        float v0 = values[0];
        ImVec2 tp0 = ImVec2(t0, 1.0f - ImSaturate((v0 - scale_min) * inv_scale));

        uint8_t color_cyan[] = {0, 237, 255};
        uint8_t color_scale[] = {int(255 * 0.4), int(255 * 0.4), int(255 * 0.4)};
        uint8_t color_scale2[] = {int(255 * 0.4), int(255 * 0.4), int(255 * 0.4)};

        // Draw lines
        float vscale = ((scale_max - scale_min) / scale_resolution);
        float step = size_y / scale_resolution;
        float value = scale_min;
        for (float i = size_y; i >= 0; i -= step)
        {
            ImVec2 pos0 = {0, i};
            ImVec2 pos1 = {(float)(size_x - 1), i};
            img.draw_line(pos0.x, pos0.y, pos1.x, pos1.y, color_scale);
            value += vscale;
            img.draw_text(pos0.x, pos0.y + 2, color_scale, 10, std::string(std::to_string(int(value)) + " dB").c_str());
        }

#if 0
        // Draw Freq Scale
        if (enable_freq_scale && bandwidth != 0 && frequency != 0)
        {
            float y_level = size_x - 8;
            img.draw_line(inner_bb.Min.x, y_level, inner_bb.Max.x, y_level, color_cyan, 3);
            for (int i = 1; i < 10; i++)
            {
                float x_line = inner_bb.Min.x + (i / 10.0) * res_w;
                double freq_here = frequency + ((i / 10.0) * bandwidth) - (bandwidth / 2);

                img.draw_line(x_line, y_level, x_line, y_level - 10, color_cyan, 3);

                auto cstr = format_notated(freq_here);
                img.draw_text(x_line - ImGui::CalcTextSize(cstr.c_str()).x / 2, y_level - 30,
                              i == 5 ? color_scale2 : color_cyan, 10 cstr.c_str());
                if (i == 5 && actual_sdr_freq != -1)
                {
                    auto cstr = "(" + format_notated(actual_sdr_freq) + ")";
                    img.draw_text({x_line - ImGui::CalcTextSize(cstr.c_str()).x / 2, y_level - 16},
                                  i == 5 ? color_scale2 : color_cyan, 10,
                                  cstr.c_str());
                }
            }
        }
#endif

        // Draw lines
        double fz = (double)values_size / (double)res_w;
        for (int n = 0; n < res_w; n++)
        {
            const float t1 = t0 + t_step;
            // const int v1_idx = (int)(t0 * item_count + 0.5f);

            float ffpos = n * fz;

            if (ffpos >= values_size)
                ffpos = values_size - 1;

            float finals = -INFINITY;
            for (float v = ffpos; v < ffpos + fz; v += 1)
                if (finals < values[(int)floor(v)])
                    finals = values[(int)floor(v)];

            const float v1 = finals; // values[v1_idx];
            const ImVec2 tp1 = ImVec2(t1, 1.0f - ImSaturate((v1 - scale_min) * inv_scale));

            ImVec2 pos0 = ImLerp({0, 0}, {(float)size_x, (float)size_y}, tp0);
            ImVec2 pos1 = ImLerp({0, 0}, {(float)size_x, (float)size_y}, tp1);

            img.draw_line(pos0.x, pos0.y, pos1.x, pos1.y, color_cyan);

            t0 = t1;
            tp0 = tp1;
        }

        work_mutex.unlock();
        return img;
    }
}