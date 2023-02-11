#pragma once

#include "imgui/imgui.h"
#include <cstdint>
#include <vector>
#include <mutex>
#include "common/colormaps.h"

namespace widgets
{
    class WaterfallPlot
    {
    private:
        int fft_max_size;
        int fft_size;
        int fft_lines;
        const int resolution = 2000; // Number of colors
        unsigned int texture_id = 0;
        uint32_t *raw_img_buffer = nullptr;

        std::vector<uint32_t> palette;

        std::mutex work_mtx;

        int last_curr_width = -1;
        int last_curr_height = -1;

        int curr_width;
        int curr_height;

        bool need_update;

        int waterfall_i_mod = 0;
        int waterfall_i = 0;

    public:
        float scale_min, scale_max;

    public:
        WaterfallPlot(int size, int lines);
        ~WaterfallPlot();
        void draw(ImVec2 size, bool active = true);

        void set_size(int size)
        {
            work_mtx.lock();
            if (size <= fft_max_size)
                fft_size = size;
            work_mtx.unlock();
        }

        void push_fft(float *values);

        void set_rate(int input_rate, int output_rate);

        void set_palette(colormaps::Map palette, bool mtx = true);
    };
}