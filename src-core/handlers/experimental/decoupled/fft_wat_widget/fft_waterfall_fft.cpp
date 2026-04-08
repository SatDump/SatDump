#include "core/style.h"
#include "fft_waterfall.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include <mutex>

namespace satdump
{
    namespace widgets
    {
        struct VFO
        {
            double f;
            double b;
            bool l = 1, u = 1;

            bool is_dragging = false;
            bool is_resizing = false;
        };

        std::vector<VFO> vfo_freqs = {{100.5e6, 100e3}, {98e6, 500e3, 0, 1}, {99.8e6, -1}};

        bool is_dragging_vfo = false;
        bool is_resizing_vfo = false;

        void FFTWaterfallWidget::draw_fft(ImVec2 pos, ImVec2 size)
        {
            std::scoped_lock l(fft_values_mtx);

            ImGuiContext &g = *GImGui;
            const ImGuiStyle &style = g.Style;
            const ImRect frame_bb(pos, {pos.x + size.x, pos.y + size.y});

            int res_w = size.x; // /*ImMin((int)size.x,*/ values_size /*)*/ - 1;
            // int item_count = values_size - 1;
            const float t_step = 1.0f / (float)res_w;
            const float inv_scale = (fft_scale_min == fft_scale_max) ? 0.0f : (1.0f / (fft_scale_max - fft_scale_min));
            float v0 = fft_values[0];
            float t0 = 0.0f;
            ImVec2 tp0 = ImVec2(t0, 1.0f - ImSaturate((v0 - fft_scale_min) * inv_scale));
            const ImU32 col_base = ImColor(21, 255, 80); // ImGui::GetColorU32(ImGuiCol_PlotLines);

            // Background
            ImGui::RenderFrame(frame_bb.Min, frame_bb.Max, ImColor(0, 0, 0), true, style.FrameRounding);

            // Draw lines
            float vscale = ((fft_scale_max - fft_scale_min) / fft_scale_resolution);
            float step = (frame_bb.Max.y - frame_bb.Min.y) / fft_scale_resolution;
            float value = fft_scale_min;
            for (float i = frame_bb.Max.y - step; i >= frame_bb.Min.y; i -= step)
            {
                ImVec2 pos0 = {frame_bb.Min.x, i};
                ImVec2 pos1 = {frame_bb.Max.x, i};
                ImGui::GetWindowDrawList()->AddLine(pos0, pos1, style::theme.fft_graduations);
                value += vscale;
                ImGui::GetWindowDrawList()->AddText({pos0.x, pos0.y + 2}, style::theme.fft_graduations, std::string(std::to_string(int(value)) + " dB").c_str());
            }

            // Draw lines
            double fz = (double)fft_values_size / (double)res_w;
            for (int n = 0; n < res_w; n++)
            {
                const float t1 = t0 + t_step;
                // const int v1_idx = (int)(t0 * item_count + 0.5f);

                float ffpos = n * fz;

                if (ffpos >= fft_values_size)
                    ffpos = fft_values_size - 1;

                float finals = -INFINITY;
                for (float v = ffpos; v < ffpos + fz; v += 1)
                    if (finals < fft_values[(int)floor(v)])
                        finals = fft_values[(int)floor(v)];

                const float v1 = finals; // values[v1_idx];
                const ImVec2 tp1 = ImVec2(t1, 1.0f - ImSaturate((v1 - fft_scale_min) * inv_scale));

                ImVec2 pos0 = ImLerp(frame_bb.Min, frame_bb.Max, tp0);
                ImVec2 pos1 = ImLerp(frame_bb.Min, frame_bb.Max, tp1);

                ImGui::GetWindowDrawList()->AddLine(pos0, pos1, col_base);

                t0 = t1;
                tp0 = tp1;
            }

            for (auto &vfo : vfo_freqs)
            {
                float pos_x = pos.x + ((vfo.f - (frequency - bandwidth / 2.0)) / bandwidth) * size.x;
                float pos_x1 = pos_x, pos_x2 = pos_x;

                if (vfo.b != -1)
                {
                    if (vfo.l)
                        pos_x1 = pos.x + (((vfo.f - (vfo.b / 2)) - (frequency - bandwidth / 2.0)) / bandwidth) * size.x;
                    if (vfo.u)
                        pos_x2 = pos.x + (((vfo.f + (vfo.b / 2)) - (frequency - bandwidth / 2.0)) / bandwidth) * size.x;

                    if (vfo.l)
                        ImGui::GetWindowDrawList()->AddRectFilled({pos_x, pos.y}, {pos_x1, pos.y + size.y}, ImColor(255, 255, 255, 100));
                    if (vfo.u)
                        ImGui::GetWindowDrawList()->AddRectFilled({pos_x, pos.y}, {pos_x2, pos.y + size.y}, ImColor(255, 255, 255, 100));
                }
                else
                {
                    pos_x1 = pos_x - 0.025 * size.x;
                    pos_x2 = pos_x + 0.025 * size.x;
                    ImGui::GetWindowDrawList()->AddRectFilledMultiColor({pos_x, pos.y}, {pos_x1, pos.y + size.y},               //
                                                                        ImColor(255, 255, 255, 100), ImColor(255, 255, 255, 0), //
                                                                        ImColor(255, 255, 255, 0), ImColor(255, 255, 255, 100));
                    ImGui::GetWindowDrawList()->AddRectFilledMultiColor({pos_x, pos.y}, {pos_x2, pos.y + size.y},               //
                                                                        ImColor(255, 255, 255, 100), ImColor(255, 255, 255, 0), //
                                                                        ImColor(255, 255, 255, 0), ImColor(255, 255, 255, 100));
                }

                // Center red line
                ImGui::GetWindowDrawList()->AddLine({pos_x, pos.y}, {pos_x, pos.y + size.y}, ImColor(255, 0, 0));

                // Resizing logic, bandwidth

                if (!vfo.is_dragging && !is_dragging_vfo && vfo.b != -1)
                {
                    if ((vfo.l && ImGui::IsMouseHoveringRect({pos_x1 - 5 * ui_scale, pos.y}, //
                                                             {pos_x1 + 5 * ui_scale, pos.y + size.y})) ||
                        (vfo.u && ImGui::IsMouseHoveringRect({pos_x2 - 5 * ui_scale, pos.y}, //
                                                             {pos_x2 + 5 * ui_scale, pos.y + size.y})))
                    {
                        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

                        if (!is_resizing_vfo && ImGui::IsMouseDown(ImGuiMouseButton_Left))
                            is_resizing_vfo = vfo.is_resizing = true;
                    }

                    if (vfo.is_resizing)
                    {
                        if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
                            vfo.b = fabs(vfo.f - ((((ImGui::GetMousePos().x - pos.x) / size.x) * bandwidth) + (frequency - bandwidth / 2.0))) * 2;
                        else
                            is_resizing_vfo = vfo.is_resizing = false;
                    }
                }

                // Dragging logic, frequency

                if (!vfo.is_resizing && !is_resizing_vfo)
                {
                    if (ImGui::IsMouseHoveringRect({std::min<float>(pos_x1 + 5 * ui_scale, pos_x - 5 * ui_scale), pos.y}, //
                                                   {std::max<float>(pos_x2 - 5 * ui_scale, pos_x + 5 * ui_scale), pos.y + size.y}))
                    {
                        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

                        if (!is_dragging_vfo && ImGui::IsMouseDown(ImGuiMouseButton_Left))
                            is_dragging_vfo = vfo.is_dragging = true;
                    }

                    if (vfo.is_dragging)
                    {
                        if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
                            vfo.f = (((ImGui::GetMousePos().x - pos.x) / size.x) * bandwidth) + (frequency - bandwidth / 2.0);
                        else
                            is_dragging_vfo = vfo.is_dragging = false;
                    }
                }
            }
        }
    } // namespace widgets
} // namespace satdump