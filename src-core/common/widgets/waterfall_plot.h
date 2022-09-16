#pragma once

#include "imgui/imgui.h"
#include <cstdint>
#include <vector>

namespace widgets
{
    class WaterfallPlot
    {
    private:
        float *values;
        int fft_size;
        int fft_lines;
        const int resolution = 2000; // Number of colors
        unsigned int texture_id;
        uint32_t *raw_img_buffer;

        std::vector<uint32_t> palette;

        bool first_run = true;

    public:
        float scale_min, scale_max;

    public:
        WaterfallPlot(float *v, int size, int lines);
        ~WaterfallPlot();
        void draw(ImVec2 size, bool active = true);
    };
}