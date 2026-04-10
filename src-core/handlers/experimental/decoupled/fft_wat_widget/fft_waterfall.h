#pragma once

#include "common/colormaps.h"
#include "imgui/imgui.h"
#include <mutex>
#include <volk/volk_alloc.hh>

namespace satdump
{
    namespace widgets
    {
        class FFTWaterfallWidget
        {
        private:
            const float fft_scale_resolution = 10;

        private:
            float *fft_values = nullptr;
            int fft_values_size = 0;
            std::mutex fft_values_mtx;

        private:
            int waterfall_max_size;
            int waterfall_size;
            int waterfall_lines;
            const int waterfall_resolution = 2000; // Number of colors
            unsigned int waterfall_texture_id = 0;
            std::vector<uint32_t> waterfall_raw_img_buffer;

            std::vector<uint32_t> waterfall_palette;

            std::mutex waterfall_mtx;

            int waterfall_last_curr_width = 0;
            int waterfall_last_curr_height = 0;

            int waterfall_curr_width;
            int waterfall_curr_height;

            bool waterfall_need_update = false;

            int waterfall_i_mod = 0;
            int waterfall_i = 0;

            bool waterfall_buffer_alloc(size_t size);

        public:
            ImColor fft_background_color = ImColor(0, 0, 0);
            ImColor fft_lines_color = ImColor(21, 255, 80);

            float fft_scale_min = -200;
            float fft_scale_max = 200;

        public:
            float waterfall_scale_min = -200;
            float waterfall_scale_max = 200;

        public:
            double bandwidth = 6e6;
            double frequency = 100e6;

            bool show_freq_scale = true;
            bool show_waterfall = true;

            bool allow_user_interactions = true;
            double fft_ratio = 0.3;

        protected:
            void draw_fft(ImVec2 pos, ImVec2 size);
            void draw_freq(ImVec2 pos, ImVec2 size);
            void draw_waterfall(ImVec2 pos, ImVec2 size);

        public:
            FFTWaterfallWidget(int waterfall_size, int waterfall_lines)
            {
                this->waterfall_max_size = this->waterfall_size = waterfall_size;
                this->waterfall_lines = waterfall_lines;
            }

            ~FFTWaterfallWidget() {}

            void draw(ImVec2 size, bool waterfall);

        public:
            void set_fft_size(int size)
            {
                fft_values_mtx.lock();
                fft_values_size = size;
                fft_values_mtx.unlock();
            }

            void set_fft_ptr(float *v)
            {
                fft_values_mtx.lock();
                fft_values = v;
                fft_values_mtx.unlock();
            }

        public:
            void set_waterfall_size(int size)
            {
                waterfall_mtx.lock();
                if (size <= waterfall_max_size)
                    waterfall_size = size;
                waterfall_mtx.unlock();
            }

            void push_waterfall_fft(float *values);

            void set_waterfall_rate(int input_rate, int output_rate);

            void set_waterfall_palette(colormaps::Map palette, bool mtx = true);
        };
    } // namespace widgets
} // namespace satdump