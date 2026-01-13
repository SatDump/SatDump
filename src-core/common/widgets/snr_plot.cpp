#include "core/style.h"
#include <array>
#include <cstring>
#include "snr_plot.h"
#include "themed_widgets.h"

namespace widgets
{
    void SNRPlotViewer::draw(float snr, float peak_snr)
    {
        ImGui::Text("SNR (dB) : ");
        ImGui::SameLine();
        ImGui::TextColored(snr > 2 ? snr > 10 ? style::theme.green : style::theme.orange : style::theme.red, "%0.3f", snr);

        ImGui::Text("Peak SNR (dB) : ");
        ImGui::SameLine();
        ImGui::TextColored(peak_snr > 2 ? peak_snr > 10 ? style::theme.green : style::theme.orange : style::theme.red, "%0.3f", peak_snr);

        std::memmove(&snr_history[0], &snr_history[1], (200 - 1) * sizeof(float));
        snr_history[200 - 1] = snr;

        // Average of the snr_history
        float avg_snr = 0.0f;
        for (int i = 0; i < 200; i++)
        {
            if (snr_history[i] >= 0 && snr_history[i] <= peak_snr){
                avg_snr += snr_history[i];
            }
        }
        avg_snr = avg_snr / 200; 

        ImGui::Text("Avg SNR (dB) : ");
        ImGui::SameLine();
        ImGui::TextColored(avg_snr > 2 ? avg_snr > 10 ? style::theme.green : style::theme.orange : style::theme.red, "%0.3f", avg_snr);

        widgets::ThemedPlotLines(style::theme.plot_bg.Value, "##snrplot", snr_history, IM_ARRAYSIZE(snr_history), 0, "", 0.0f, 25.0f,
            ImVec2(200 * ui_scale, 50 * ui_scale));
    }
}