#include "common/colormaps.h"
#include "core/resources.h"
#include "core/style.h"
#include "fft_waterfall.h"
#include "imgui/imgui.h"
#include "imgui/imgui_image.h"
#include "imgui/imgui_internal.h"
#include "logger.h"
#include <mutex>

namespace satdump
{
    namespace widgets
    {
        bool FFTWaterfallWidget::waterfall_buffer_alloc(size_t size)
        {
            uint32_t *new_img_buffer = (uint32_t *)realloc(waterfall_raw_img_buffer, size);
            if (new_img_buffer == nullptr)
            {
                logger->error("Cannot allocate memory for waterfall");
                if (waterfall_raw_img_buffer != nullptr)
                {
                    free(waterfall_raw_img_buffer);
                    waterfall_raw_img_buffer = nullptr;
                }
                waterfall_last_curr_height = waterfall_last_curr_width = 0;
                return false;
            }

            waterfall_raw_img_buffer = new_img_buffer;
            uint64_t old_size = waterfall_last_curr_width * waterfall_last_curr_height;
            if (size > old_size * sizeof(uint32_t))
                memset(&waterfall_raw_img_buffer[old_size], 0, size - old_size * sizeof(uint32_t));
            waterfall_last_curr_width = waterfall_curr_width;
            waterfall_last_curr_height = waterfall_curr_height;
            return true;
        }

        void FFTWaterfallWidget::draw_waterfall(ImVec2 pos, ImVec2 size)
        {
            waterfall_mtx.lock();
            if (true) // if (waterfall_texture_id == 0 /*|| active*/)
            {
                waterfall_curr_width = size.x > waterfall_size ? waterfall_size : size.x;
                waterfall_curr_height = size.y > waterfall_lines ? waterfall_lines : size.y;
            }
            if (waterfall_texture_id == 0)
            {
                waterfall_texture_id = makeImageTexture();
                waterfall_need_update = waterfall_buffer_alloc(waterfall_curr_width * waterfall_curr_height * sizeof(uint32_t));
                if ((int)waterfall_palette.size() != waterfall_resolution)
                    set_waterfall_palette(colormaps::loadMap(resources::getResourcePath("waterfall/classic.json")), false);
            }
            if (/*active &&*/ (waterfall_last_curr_width != waterfall_curr_width || waterfall_last_curr_height != waterfall_curr_height))
            {
                if (waterfall_raw_img_buffer != nullptr && waterfall_last_curr_width != waterfall_curr_width)
                {
                    free(waterfall_raw_img_buffer);
                    waterfall_raw_img_buffer = nullptr;
                    waterfall_last_curr_height = waterfall_last_curr_width = 0;
                }
                waterfall_need_update = waterfall_buffer_alloc(waterfall_curr_width * waterfall_curr_height * sizeof(uint32_t));
            }
            if (waterfall_need_update)
            {
                updateImageTexture(waterfall_texture_id, waterfall_raw_img_buffer, waterfall_curr_width, waterfall_curr_height);
                waterfall_need_update = false;
            }
            waterfall_mtx.unlock();

            ImGui::GetWindowDrawList()->AddImage((void *)(intptr_t)waterfall_texture_id, pos, pos + size);
        }

        void FFTWaterfallWidget::push_waterfall_fft(float *values)
        {
            if (waterfall_texture_id == 0 || waterfall_raw_img_buffer == nullptr)
                return;

            waterfall_mtx.lock();
            if ((waterfall_i++ % waterfall_i_mod) == 0)
            {
                if (waterfall_i * 5e6 == waterfall_i_mod)
                    waterfall_i = 0;

                memmove(&waterfall_raw_img_buffer[waterfall_curr_width * 1], &waterfall_raw_img_buffer[waterfall_curr_width * 0],
                        waterfall_curr_width * (waterfall_curr_height - 1) * sizeof(uint32_t));

                double fz = (double)waterfall_size / (double)waterfall_curr_width;
                for (int i = 0; i < waterfall_curr_width; i++)
                {
                    float ffpos = i * fz;

                    if (ffpos >= waterfall_size)
                        ffpos = waterfall_size - 1;

                    float final = -INFINITY;
                    for (float v = ffpos; v < ffpos + fz; v += 1)
                        if (final < values[(int)floor(v)])
                            final = values[(int)floor(v)];

                    int v = ((final - waterfall_scale_min) / fabs(waterfall_scale_max - waterfall_scale_min)) * waterfall_resolution;

                    if (v < 0)
                        v = 0;
                    if (v >= waterfall_resolution)
                        v = waterfall_resolution - 1;

                    waterfall_raw_img_buffer[i] = waterfall_palette[v];
                }

                waterfall_need_update = true;
            }
            waterfall_mtx.unlock();
        }

        void FFTWaterfallWidget::set_waterfall_rate(int input_rate, int output_rate)
        {
            waterfall_mtx.lock();
            if (output_rate <= 0)
                output_rate = 1;
            waterfall_i_mod = input_rate / output_rate;
            if (waterfall_i_mod <= 0)
                waterfall_i_mod = 1;
            waterfall_i = 0;
            waterfall_mtx.unlock();
        }

        void FFTWaterfallWidget::set_waterfall_palette(colormaps::Map pa, bool mtx)
        {
            if (mtx)
                waterfall_mtx.lock();
            waterfall_palette = colormaps::generatePalette(pa, waterfall_resolution);
            if (mtx)
                waterfall_mtx.unlock();
        }
    } // namespace widgets
} // namespace satdump