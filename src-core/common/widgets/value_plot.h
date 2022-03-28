#pragma once

#include "imgui/imgui.h"
#include "complex"

namespace widgets
{
    class ValuePlotViewer
    {
    private:
        float history[200];

    public:
        void draw(float value, float max, float min, std::string name);
    };
}