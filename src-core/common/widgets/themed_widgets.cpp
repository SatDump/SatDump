#include "themed_widgets.h"

namespace widgets
{
    void ThemedPlotLines(ImVec4 bg_color, const char* label, const float* values, int values_count, int values_offset, const char* overlay_textL, float scale_min, float scale_max, ImVec2 graph_size)
    {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, bg_color);
        ImGui::PlotLines(label, values, values_count, values_offset, overlay_textL, scale_min, scale_max, graph_size);
        ImGui::PopStyleColor();
    }
}