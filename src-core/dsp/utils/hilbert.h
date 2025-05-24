#pragma once

#include "common/dsp/block.h"
#include "common/dsp/complex.h"
#include "common/dsp/filter/firdes.h"
#include "dsp/block.h"
#include <volk/volk.h>

namespace satdump
{
    namespace ndsp
    {
        class HilbertBlock : public Block
        {
        private:
            bool needs_reinit = false;
            int p_ntaps = 65;
            int p_buffer_size = 8192 * 8;

        private:
            int buffer_size = 0;
            float *buffer = nullptr;
            float *taps = nullptr;

            bool work();

            size_t lbuf_size;
            size_t lbuf_offset;

        public:
            HilbertBlock();
            ~HilbertBlock();

            void init()
            {
                // Free if needed
                if (taps != nullptr)
                    volk_free(taps);
                if (buffer != nullptr)
                    volk_free(buffer);

                // Init buffer
                buffer = dsp::create_volk_buffer<float>(p_buffer_size); // TODOREWORK How to handle this from the initial buffer size?
                buffer_size = p_buffer_size;

                // Init taps
                auto p_taps = dsp::firdes::hilbert(p_ntaps, dsp::fft::window::WIN_HAMMING, 6.76);

                // Init taps
                p_ntaps = p_taps.size();
                taps = (float *)volk_malloc(p_ntaps * sizeof(float), volk_get_alignment());
                for (int j = 0; j < p_ntaps; j++)
                    taps[j] = p_taps[(p_ntaps - 1) - j]; // Reverse taps
            }

            nlohmann::ordered_json get_cfg_list()
            {
                nlohmann::ordered_json p;
                add_param_simple(p, "ntaps", "int");
                return p;
            }

            nlohmann::json get_cfg(std::string key)
            {
                if (key == "ntaps")
                    return p_ntaps;
                else if (key == "buffer_size")
                    return p_buffer_size;
                else
                    throw satdump_exception(key);
            }

            cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                if (key == "ntaps")
                {
                    p_ntaps = v;
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
