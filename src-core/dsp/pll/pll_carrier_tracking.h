#pragma once

#include "common/dsp/complex.h"
#include "dsp/block_simple.h"

namespace satdump
{
    namespace ndsp
    {
        class PLLCarrierTrackingBlock : public BlockSimple<complex_t, complex_t>
        {
        private:
            float p_loop_bw = 0.01;
            float p_freq_max = 1;
            float p_freq_min = -1;

        private:
            float d_max_freq, d_min_freq;
            float d_damping, d_loop_bw;
            float d_alpha, d_beta;
            float d_phase, d_freq;

        public:
            uint32_t process(complex_t *input, uint32_t nsamples, complex_t *output);

        public:
            PLLCarrierTrackingBlock();
            ~PLLCarrierTrackingBlock();

            void init()
            {
                d_damping = sqrtf(2.0f) / 2.0f;

                float denom = (1.0 + 2.0 * d_damping * d_loop_bw + d_loop_bw * d_loop_bw);
                d_alpha = (4 * d_damping * d_loop_bw) / denom;
                d_beta = (4 * d_loop_bw * d_loop_bw) / denom;
            }

            nlohmann::ordered_json get_cfg_list()
            {
                nlohmann::ordered_json p;
                add_param_simple(p, "loop_bw", "float");
                add_param_simple(p, "freq_max", "float");
                add_param_simple(p, "freq_min", "float");
                return p;
            }

            nlohmann::json get_cfg(std::string key)
            {
                if (key == "loop_bw")
                    return p_loop_bw;
                else if (key == "freq_max")
                    return p_freq_max;
                else if (key == "freq_min")
                    return p_freq_min;
                else
                    throw satdump_exception(key);
            }

            cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                if (key == "loop_bw")
                {
                    p_loop_bw = v;
                    init();
                }
                else if (key == "freq_max")
                {
                    p_freq_max = v;
                    init();
                }
                else if (key == "freq_min")
                {
                    p_freq_min = v;
                    init();
                }
                else
                    throw satdump_exception(key);
                return RES_OK;
            }
        };
    } // namespace ndsp
} // namespace satdump