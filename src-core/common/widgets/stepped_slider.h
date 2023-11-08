#pragma once

#include "imgui/imgui.h"

namespace widgets
{
    bool SteppedSliderInt(const char* label, int* v, int v_min, int v_max, int v_rate = 1, const char* format = "%d", ImGuiSliderFlags flags = 0);
    bool SteppedSliderFloat(const char* label, float* v, float v_min, float v_max, float v_rate = 1.0f, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
}