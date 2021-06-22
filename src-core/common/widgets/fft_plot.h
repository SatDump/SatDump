#pragma once

#include "imgui/imgui.h"

namespace widgets
{
    class FFTPlot
    {
    private:
        float *values;
        int values_size;

    public:
        float scale_min, scale_max;

    public:
        FFTPlot(float *v, int size, float min, float max);
        void draw(ImVec2 size);
    };
}