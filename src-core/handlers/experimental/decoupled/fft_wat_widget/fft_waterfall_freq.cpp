#include "common/dsp_source_sink/format_notated.h"
#include "core/style.h"
#include "fft_waterfall.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include <mutex>

namespace satdump
{
    namespace widgets
    {
        void FFTWaterfallWidget::draw_freq(ImVec2 pos, ImVec2 fsize)
        {
            ImGui::GetWindowDrawList()->AddRectFilled( //
                {pos.x, pos.y},                        //
                {pos.x + fsize.x, pos.y + fsize.y},    //
                /*ImColor(ImGui::GetStyle().Colors[ImGuiCol_Button])*/ ImColor(0, 0, 0));

            // Start
            {
                std::string cfreq = " " + format_notated(frequency - (bandwidth / 2.), "Hz");
                auto size = ImGui::CalcTextSize(cfreq.c_str());
                ImGui::GetWindowDrawList()->AddText({pos.x, pos.y + (fsize.y - size.y) / 2.f}, ImColor(ImGui::GetStyle().Colors[ImGuiCol_Text]), cfreq.c_str());
            }

            // Start - Center
            {
                std::string cfreq = format_notated(frequency - (bandwidth / 4.), "Hz");
                auto size = ImGui::CalcTextSize(cfreq.c_str());
                ImGui::GetWindowDrawList()->AddText({pos.x + (fsize.x / 4) - (size.x / 2), pos.y + (fsize.y - size.y) / 2.f}, ImColor(ImGui::GetStyle().Colors[ImGuiCol_Text]), cfreq.c_str());
            }

            // Center
            {
                std::string cfreq = format_notated(frequency, "Hz");
                auto size = ImGui::CalcTextSize(cfreq.c_str());
                ImGui::GetWindowDrawList()->AddText({pos.x + (fsize.x / 2) - (size.x / 2), pos.y + (fsize.y - size.y) / 2.f}, ImColor(ImGui::GetStyle().Colors[ImGuiCol_Text]), cfreq.c_str());
            }

            // Center - End
            {
                std::string cfreq = format_notated(frequency - (bandwidth / 4.) * 3., "Hz");
                auto size = ImGui::CalcTextSize(cfreq.c_str());
                ImGui::GetWindowDrawList()->AddText({pos.x + (fsize.x / 4) * 3 - (size.x / 2), pos.y + (fsize.y - size.y) / 2.f}, ImColor(ImGui::GetStyle().Colors[ImGuiCol_Text]), cfreq.c_str());
            }

            // End
            {
                std::string cfreq = format_notated(frequency + (bandwidth / 2.), "Hz") + " ";
                auto size = ImGui::CalcTextSize(cfreq.c_str());
                ImGui::GetWindowDrawList()->AddText({pos.x + fsize.x - size.x, pos.y + (fsize.y - size.y) / 2.f}, ImColor(ImGui::GetStyle().Colors[ImGuiCol_Text]), cfreq.c_str());
            }
        }
    } // namespace widgets
} // namespace satdump