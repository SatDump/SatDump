#include "waterfall_plot.h"
#include "imgui/imgui_internal.h"
#include <string>
#include "imgui/imgui_image.h"
#include "resources.h"

namespace widgets
{
    WaterfallPlot::WaterfallPlot(float *v, int size, int lines)
    {
        values = v;
        fft_max_size = fft_size = size;
        fft_lines = lines;
    }

    WaterfallPlot::~WaterfallPlot()
    {
        if (!first_run)
            delete[] raw_img_buffer;
    }

    void WaterfallPlot::draw(ImVec2 size, bool active)
    {
        curr_width = size.x > fft_size ? fft_size : size.x;
        curr_height = size.y > fft_lines ? fft_lines : size.y;

        work_mtx.lock();
        if (active || first_run)
        {
            if (first_run)
            {
                texture_id = makeImageTexture();
                first_run = false;
                raw_img_buffer = new uint32_t[fft_size * fft_lines];
                memset(raw_img_buffer, 0, sizeof(uint32_t) * fft_size * fft_lines);
                if ((int)palette.size() != resolution)
                    set_palette(colormaps::loadMap(resources::getResourcePath("waterfall/classic.json")), false);
            }

            if (last_curr_width != curr_width || last_curr_height != curr_height)
            {
                if (raw_img_buffer != nullptr)
                    delete[] raw_img_buffer;
                raw_img_buffer = new uint32_t[fft_size * fft_lines];
                memset(raw_img_buffer, 0, sizeof(uint32_t) * curr_width * curr_height);
                last_curr_width = curr_width;
                last_curr_height = curr_height;
            }

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

            updateImageTexture(texture_id, raw_img_buffer, curr_width, curr_height);
        }

        ImGui::Image((void *)(intptr_t)texture_id, size);
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