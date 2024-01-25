#pragma once
#include "imgui/imgui.h"

namespace widgets
{
    void ThemedPlotLines(ImVec4 bg_color, const char* label, const float* values, int values_count, int values_offset = 0, const char* overlay_text = NULL, float scale_min = FLT_MAX, float scale_max = FLT_MAX, ImVec2 graph_size = ImVec2(0, 0));
}