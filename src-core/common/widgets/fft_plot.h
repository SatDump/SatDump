#pragma once

#include "imgui/imgui.h"
#include <mutex>
#include "image/image.h"
#include <string>

namespace widgets
{
    class FFTPlot
    {
    private:
        float *values;
        int values_size;
        std::mutex work_mutex;

    public:
        float scale_min, scale_max;
        float scale_resolution;

        double bandwidth = 0;
        double frequency = 0;

        bool enable_freq_scale = true;

        double actual_sdr_freq = -1;

    public:
        FFTPlot(float *v, int size, float min, float max, float scale_res = 20);

        void draw(ImVec2 size);
        image::Image drawImg(int size_x, int size_y);

        void set_size(int size)
        {
            work_mutex.lock();
            values_size = size;
            work_mutex.unlock();
        }

        std::vector<std::pair<std::string, double>> vfo_freqs;
    };
}