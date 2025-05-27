#include "waterfall_plot.h"
#include "imgui/imgui_internal.h"
#include <string>
#include "imgui/imgui_image.h"
#include "core/resources.h"
#include "logger.h"

namespace widgets
{
    WaterfallPlot::WaterfallPlot(int size, int lines)
    {
        fft_max_size = fft_size = size;
        fft_lines = lines;
    }

    WaterfallPlot::~WaterfallPlot()
    {
        if (raw_img_buffer != nullptr)
            free(raw_img_buffer);
    }

    bool WaterfallPlot::buffer_alloc(size_t size)
    {
        uint32_t* new_img_buffer = (uint32_t*)realloc(raw_img_buffer, size);
        if (new_img_buffer == nullptr)
        {
            logger->error("Cannot allocate memory for waterfall");
            if (raw_img_buffer != nullptr)
            {
                free(raw_img_buffer);
                raw_img_buffer = nullptr;
            }
            last_curr_height = last_curr_width = 0;
            return false;
        }

        raw_img_buffer = new_img_buffer;
        uint64_t old_size = last_curr_width * last_curr_height;
        if (size > old_size * sizeof(uint32_t))
            memset(&raw_img_buffer[old_size], 0, size - old_size * sizeof(uint32_t));
        last_curr_width = curr_width;
        last_curr_height = curr_height;
        return true;
    }

    void WaterfallPlot::draw(ImVec2 size, bool active)
    {
        work_mtx.lock();
        if (texture_id == 0 || active)
        {
            curr_width = size.x > fft_size ? fft_size : size.x;
            curr_height = size.y > fft_lines ? fft_lines : size.y;
        }
        if (texture_id == 0)
        {
            texture_id = makeImageTexture();
            need_update = buffer_alloc(curr_width * curr_height * sizeof(uint32_t));
            if ((int)palette.size() != resolution)
                set_palette(colormaps::loadMap(resources::getResourcePath("waterfall/classic.json")), false);
        }
        if (active && (last_curr_width != curr_width || last_curr_height != curr_height))
        {
            if (raw_img_buffer != nullptr && last_curr_width != curr_width)
            {
                free(raw_img_buffer);
                raw_img_buffer = nullptr;
                last_curr_height = last_curr_width = 0;
            }
            need_update = buffer_alloc(curr_width * curr_height * sizeof(uint32_t));
        }
        if (need_update)
        {
            updateImageTexture(texture_id, raw_img_buffer, curr_width, curr_height);
            need_update = false;
        }
        work_mtx.unlock();

        ImGui::Image((void *)(intptr_t)texture_id, size);
    }

    void WaterfallPlot::push_fft(float *values)
    {
        if (texture_id == 0 || raw_img_buffer == nullptr)
            return;

        work_mtx.lock();
        if ((waterfall_i++ % waterfall_i_mod) == 0)
        {
            if (waterfall_i * 5e6 == waterfall_i_mod)
                waterfall_i = 0;

            memmove(&raw_img_buffer[curr_width * 1], &raw_img_buffer[curr_width * 0], curr_width * (curr_height - 1) * sizeof(uint32_t));

            double fz = (double)fft_size / (double)curr_width;
            for (int i = 0; i < curr_width; i++)
            {
                float ffpos = i * fz;

                if (ffpos >= fft_size)
                    ffpos = fft_size - 1;

                float final = -INFINITY;
                for (float v = ffpos; v < ffpos + fz; v += 1)
                    if (final < values[(int)floor(v)])
                        final = values[(int)floor(v)];

                int v = ((final - scale_min) / fabs(scale_max - scale_min)) * resolution;

                if (v < 0)
                    v = 0;
                if (v >= resolution)
                    v = resolution - 1;

                raw_img_buffer[i] = palette[v];
            }

            need_update = true;
        }
        work_mtx.unlock();
    }

    void WaterfallPlot::set_rate(int input_rate, int output_rate)
    {
        work_mtx.lock();
        if (output_rate <= 0)
            output_rate = 1;
        waterfall_i_mod = input_rate / output_rate;
        if (waterfall_i_mod <= 0)
            waterfall_i_mod = 1;
        waterfall_i = 0;
        work_mtx.unlock();
    }

    void WaterfallPlot::set_palette(colormaps::Map pa, bool mtx)
    {
        if (mtx)
            work_mtx.lock();
        palette = colormaps::generatePalette(pa, resolution);
        if (mtx)
            work_mtx.unlock();
    }
}