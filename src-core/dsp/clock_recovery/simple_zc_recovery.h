#pragma once

#include "dsp/block_simple.h"

namespace satdump
{
    namespace ndsp
    {
        class SimpleZeroCrossingRecoveryBlock : public BlockSimple<float, float>
        {
        private:
            float prev = 0.0f;
            float phase = 0.0f;
            float period = 5;
            float alpha = 0.1f;

        private:
            bool clock_recovery(float sample, float *sym);

        public:
            uint32_t process(float *input, uint32_t nsamples, float *output);

        public:
            SimpleZeroCrossingRecoveryBlock();
            ~SimpleZeroCrossingRecoveryBlock();

            void init();

            nlohmann::ordered_json get_cfg_list()
            {
                nlohmann::ordered_json p;
                add_param_simple(p, "sps", "float");
                add_param_simple(p, "alpha", "float");
                return p;
            }

            nlohmann::json get_cfg(std::string key)
            {
                if (key == "sps")
                    return period;
                else if (key == "alpha")
                    return alpha;
                else
                    throw satdump_exception(key);
            }

            Block::cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                if (key == "sps")
                    period = v;
                else if (key == "alpha")
                    alpha = v;
                else
                    throw satdump_exception(key);
                return Block::RES_OK;
            }
        };
    } // namespace ndsp
} // namespace satdump