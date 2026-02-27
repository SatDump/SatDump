#pragma once

#include "common/dsp/block.h"
#include "common/dsp/complex.h"
#include "dsp/block.h"
#include "dsp/block_simple.h"
#include <volk/volk_alloc.hh>

namespace satdump
{
    namespace ndsp
    {
        template <typename T>
        class FIRBlock : public BlockSimple<T, T>
        {
        public:
            bool needs_reinit = false;
            std::vector<float> p_taps;

        private:
            int buffer_size = 0;
            volk::vector<T> buffer;
            float **taps = nullptr;
            int ntaps;
            int align;
            int aligned_tap_count;

            size_t in_buffer;

        public:
            uint32_t process(T *input, uint32_t nsamples, T *output);

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

                // Init buffer
                buffer.resize(8192);
                buffer_size = buffer.size();
                in_buffer = 0;

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
                else
                    throw satdump_exception(key);
            }

            Block::cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                if (key == "taps")
                {
                    p_taps = v.get<std::vector<float>>();
                    needs_reinit = true;
                }
                else
                    throw satdump_exception(key);
                return Block::RES_OK;
            }
        };
    } // namespace ndsp
} // namespace satdump