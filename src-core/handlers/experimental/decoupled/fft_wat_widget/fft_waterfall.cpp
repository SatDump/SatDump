#include "fft_waterfall.h"
#include "core/style.h"
#include "imgui/imgui.h"

namespace satdump
{
    namespace widgets
    {
        void FFTWaterfallWidget::draw(ImVec2 size, bool waterfall)
        {
            const int freq_y = 20 * ui_scale;

            // Draw FFT
            ImVec2 fft_pos, fft_size;
            fft_pos.x = ImGui::GetCursorScreenPos().x;
            fft_pos.y = ImGui::GetCursorScreenPos().y;
            fft_size.x = size.x;
            fft_size.y = waterfall ? (size.y * fft_ratio) : (size.y - freq_y);

            draw_fft(fft_pos, fft_size);

            // Draw Freq scale
            ImVec2 fre_pos, fre_size;
            fre_pos.x = ImGui::GetCursorScreenPos().x;
            fre_pos.y = ImGui::GetCursorScreenPos().y + fft_size.y;
            fre_size.x = size.x;
            fre_size.y = freq_y;

            draw_freq(fre_pos, fre_size);

            // Resize logic
            if (allow_user_interactions && ImGui::IsMouseHoveringRect(fre_pos, fre_pos + fre_size))
            {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
                if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
                    fft_ratio = ((ImGui::GetMousePos().y - fft_pos.y - (fre_size.y / 2.f)) / size.y);
            }

            // Safety
            while (waterfall && (((size.y * fft_ratio) + fre_size.y + 1) >= size.y))
                fft_ratio *= 0.99f;
            if (waterfall && fft_ratio <= 0.0f)
                fft_ratio = 0.01f;

            // Draw Waterfall
            if (waterfall)
            {
                ImVec2 wat_pos, wat_size;
                wat_pos.x = ImGui::GetCursorScreenPos().x;
                wat_pos.y = ImGui::GetCursorScreenPos().y + fft_size.y + fre_size.y;
                wat_size.x = size.x;
                wat_size.y = size.y - fft_size.y - fre_size.y;

                draw_waterfall(wat_pos, wat_size);
            }

            ImGui::Dummy(size);
        }
    } // namespace widgets
} // namespace satdump