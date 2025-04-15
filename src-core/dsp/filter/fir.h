#pragma once

#include "common/dsp/block.h"
#include "common/dsp/complex.h"
#include "dsp/block.h"

namespace satdump
{
    namespace ndsp
    {
        template <typename T>
        class FIRBlock : public Block
        {
        public:
            bool needs_reinit = false;
            std::vector<float> p_taps;
            int p_buffer_size = 8192 * 8;

        private:
            int buffer_size = 0;
            T *buffer = nullptr;
            float **taps = nullptr;
            int ntaps;
            int align;
            int aligned_tap_count;

            bool work();

            size_t lbuf_size;
            size_t lbuf_offset;

        public:
            FIRBlock();
            ~FIRBlock();

            void init()
            {
                // Free if needed
                if (taps != nullptr)
                {
                    for (int i = 0; i < aligned_tap_count; i++)
                        volk_free(taps[i]);
                    volk_free(taps);
                }
                if (buffer != nullptr)
                    volk_free(buffer);

                // Init buffer
                buffer = dsp::create_volk_buffer<T>(p_buffer_size); // TODOREWORK How to handle this from the initial buffer size?
                buffer_size = p_buffer_size;

                // Get alignement parameters
                align = volk_get_alignment();
                aligned_tap_count = std::max<int>(1, align / sizeof(T));

                // Set taps
                ntaps = p_taps.size();

                // Init taps
                this->taps = (float **)volk_malloc(aligned_tap_count * sizeof(float *), align);
                for (int i = 0; i < aligned_tap_count; i++)
                {
                    this->taps[i] = (float *)volk_malloc((ntaps + aligned_tap_count - 1) * sizeof(float), align);
                    for (int y = 0; y < ntaps + aligned_tap_count - 1; y++)
                        this->taps[i][y] = 0;
                    for (int j = 0; j < ntaps; j++)
                        this->taps[i][i + j] = p_taps[(ntaps - 1) - j]; // Reverse taps
                }
            }

            nlohmann::json get_cfg(std::string key)
            {
                if (key == "taps")
                    return p_taps;
                else if (key == "buffer_size")
                    return p_buffer_size;
                else
                    throw satdump_exception(key);
            }

            cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                if (key == "taps")
                {
                    p_taps = v.get<std::vector<float>>();
                    needs_reinit = true;
                }
                else if (key == "buffer_size")
                    p_buffer_size = v;
                else
                    throw satdump_exception(key);
                return RES_OK;
            }
        };
    } // namespace ndsp
} // namespace satdump