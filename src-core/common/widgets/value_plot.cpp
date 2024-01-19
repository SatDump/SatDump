#include "core/module.h"
#include <array>
#include <cstring>
#include "value_plot.h"

namespace widgets
{
    void ValuePlotViewer::draw(float value, float max, float min, std::string name)
    {
        ImGui::Text("%s", name.c_str());
        ImGui::SameLine();
        ImGui::TextColored(value > -1 ? value < 5 ? IMCOLOR_GREEN : IMCOLOR_ORANGE : IMCOLOR_RED, UITO_C_STR(value));

        std::memmove(&history[0], &history[1], (200 - 1) * sizeof(float));
        history[200 - 1] = value;

        ImGui::PlotLines("", history, IM_ARRAYSIZE(history), 0, "", min, max, ImVec2(200 * ui_scale, 50 * ui_scale));
    }
}