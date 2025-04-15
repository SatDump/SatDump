#pragma once

#include "common/dsp/complex.h"
#include "dsp/block.h"

namespace satdump
{
    namespace ndsp
    {
        class CostasBlock : public Block
        {
        private:
            int order = 2;
            float loop_bw = 0.004;
            float freq_limit = 1.0;

        private:
            float error = 0;
            float phase = 0, freq = 0;
            float alpha, beta;

            float freq_limit_min, freq_limit_max;

            complex_t tmp_val;

            bool work();

        public:
            CostasBlock();
            ~CostasBlock();

            void init()
            {
                freq_limit_min = -freq_limit;
                freq_limit_max = freq_limit;

                error = 0;
                phase = 0, freq = 0;

                float damping = sqrtf(2.0f) / 2.0f;
                float denom = (1.0 + 2.0 * damping * loop_bw + loop_bw * loop_bw);
                alpha = (4 * damping * loop_bw) / denom;
                beta = (4 * loop_bw * loop_bw) / denom;
            }

            nlohmann::ordered_json get_cfg_list()
            {
                nlohmann::ordered_json p;
                add_param(p, "loop_bw", "float");
                add_param(p, "order", "int");
                add_param(p, "freq_limit", "float");
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