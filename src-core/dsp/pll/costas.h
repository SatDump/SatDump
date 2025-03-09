#pragma once

#include "dsp/block.h"
#include "common/dsp/complex.h"

namespace satdump
{
    namespace ndsp
    {
        class CostasBlock : public Block
        {
        public:
            float p_loop_bw = 0.004;
            unsigned int p_order = 2;
            float p_freq_limit = 1.0;

        private:
            float error = 0;
            int order;

            float phase = 0, freq = 0;
            float loop_bw;
            float alpha, beta;

            float freq_limit_min, freq_limit_max;

            complex_t tmp_val;

            bool work();

        public:
            CostasBlock();
            ~CostasBlock();

            void init()
            {
                order = p_order;
                loop_bw = p_loop_bw;
                freq_limit_min = -p_freq_limit;
                freq_limit_max = p_freq_limit;

                error = 0;
                phase = 0, freq = 0;

                float damping = sqrtf(2.0f) / 2.0f;
                float denom = (1.0 + 2.0 * damping * loop_bw + loop_bw * loop_bw);
                alpha = (4 * damping * loop_bw) / denom;
                beta = (4 * loop_bw * loop_bw) / denom;
            }

            nlohmann::json getParams()
            {
                nlohmann::json v;
                v["loop_bw"] = p_loop_bw;
                v["order"] = p_order;
                v["freq_limit"] = p_freq_limit;
                return v;
            }

            void setParams(nlohmann::json v)
            {
                setValFromJSONIfExists(p_loop_bw, v["loop_bw"]);
                setValFromJSONIfExists(p_order, v["order"]);
                setValFromJSONIfExists(p_freq_limit, v["freq_limit"]);
            }
        };
    }
}