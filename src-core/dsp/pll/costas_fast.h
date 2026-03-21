#pragma once

#include "common/dsp/complex.h"
#include "dsp/block_simple.h"

namespace satdump
{
    namespace ndsp
    {
        class CostasFastBlock : public BlockSimple<complex_t, complex_t>
        {
        private:
            int order = 2;
            float loop_bw = 0.004f;
            float freq_limit = 1.0f;

        private:
            float freq = 0.0f;
            float alpha, beta;

            float freq_limit_min, freq_limit_max;
            complex_t freq_limit_min_cpx, freq_limit_max_cpx;

            float pha_re = 1, pha_im = 0; // Phase, used as NCO
            float fre_re = 1, fre_im = 0; // Freq, as phase increment

            uint32_t renorm_ctr = 0;

        private:
            template <int ORDER>
            uint32_t process_order(complex_t *ibuf, uint32_t nsamples, complex_t *obuf);

        public:
            uint32_t process(complex_t *input, uint32_t nsamples, complex_t *output);

        public:
            CostasFastBlock();
            ~CostasFastBlock();

            void init()
            {
                freq_limit_min = -freq_limit;
                freq_limit_max = freq_limit;

                freq_limit_min_cpx = complex_t(cosf(freq_limit_min), sinf(freq_limit_min));
                freq_limit_max_cpx = complex_t(cosf(freq_limit_max), sinf(freq_limit_max));

                freq = 0;
                pha_re = 1;
                pha_im = 0;
                fre_re = 1;
                fre_im = 0;
                renorm_ctr = 0;

                float damping = sqrtf(2.0f) / 2.0f;
                float denom = 1.0 + 2.0 * damping * loop_bw + loop_bw * loop_bw;
                alpha = (4 * damping * loop_bw) / denom;
                beta = (4 * loop_bw * loop_bw) / denom;
            }

            nlohmann::ordered_json get_cfg_list()
            {
                nlohmann::ordered_json p;
                add_param_simple(p, "loop_bw", "float");
                add_param_simple(p, "order", "int");
                add_param_simple(p, "freq_limit", "float");
                add_param_simple(p, "freq", "stat");
                return p;
            }

            nlohmann::json get_cfg(std::string key)
            {
                if (key == "loop_bw")
                    return loop_bw;
                else if (key == "order")
                    return order;
                else if (key == "freq_limit")
                    return freq_limit;
                else if (key == "freq")
                    return freq;
                else
                    throw satdump_exception(key);
            }

            cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                if (key == "loop_bw")
                {
                    loop_bw = v;
                    init();
                }
                else if (key == "order")
                    order = v;
                else if (key == "freq")
                    ;
                else if (key == "freq_limit")
                {
                    freq_limit = v;
                    init();
                }
                else
                    throw satdump_exception(key);
                return RES_OK;
            }
        };

    } // namespace ndsp
} // namespace satdump