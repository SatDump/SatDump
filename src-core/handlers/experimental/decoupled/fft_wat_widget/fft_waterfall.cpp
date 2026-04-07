#include "fft_waterfall.h"
#include "imgui/imgui.h"

namespace satdump
{
    namespace widgets
    {
        void FFTWaterfallWidget::draw(ImVec2 size, bool waterfall)
        {
            const int freq_y = 20;

            ImVec2 fft_pos, fft_size;
            fft_pos.x = ImGui::GetCursorScreenPos().x;
            fft_pos.y = ImGui::GetCursorScreenPos().y;
            fft_size.x = size.x;
            fft_size.y = waterfall ? (size.y * 0.3) : (size.y - freq_y);

            draw_fft(fft_pos, fft_size);

            ImVec2 fre_pos, fre_size;
            fre_pos.x = ImGui::GetCursorScreenPos().x;
            fre_pos.y = ImGui::GetCursorScreenPos().y + fft_size.y;
            fre_size.x = size.x;
            fre_size.y = freq_y;

            draw_freq(fre_pos, fre_size);

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