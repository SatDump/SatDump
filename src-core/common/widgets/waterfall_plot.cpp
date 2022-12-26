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

            memmove(&raw_img_buffer[fft_size * 1], &raw_img_buffer[fft_size * 0], fft_size * (fft_lines - 1) * sizeof(uint32_t));

            for (int i = 0; i < fft_size; i++)
            {
                int v = ((values[i] - scale_min) / fabs(scale_max - scale_min)) * resolution;

                if (v < 0)
                    v = 0;
                if (v >= resolution)
                    v = resolution - 1;

                raw_img_buffer[i] = palette[v];
            }

            updateImageTexture(texture_id, raw_img_buffer, fft_size, fft_lines);
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