#pragma once

#include "dsp/block_simple.h"

namespace satdump
{
    namespace ndsp
    {
        class AGCBiasBlock : public BlockSimple<float, float>
        {
        private:
            float bias;
            float moving_avg;

            float target = 5;
            float bias_pole = 0.01;
            float gain_pole = 0.001;

        public:
            uint32_t process(float *input, uint32_t nsamples, float *output);

        public:
            AGCBiasBlock();
            ~AGCBiasBlock();

            void init();

            nlohmann::ordered_json get_cfg_list()
            {
                nlohmann::ordered_json p;
                add_param_simple(p, "target", "float");
                add_param_simple(p, "bias", "float");
                add_param_simple(p, "gain", "float");
                return p;
            }

            nlohmann::json get_cfg(std::string key)
            {
                if (key == "target")
                    return target;
                else if (key == "bias")
                    return bias_pole;
                else if (key == "gain")
                    return gain_pole;
                else
                    throw satdump_exception(key);
            }

            Block::cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                if (key == "target")
                    target = v;
                else if (key == "bias")
                    bias_pole = v;
                else if (key == "gain")
                    gain_pole = v;
                else
                    throw satdump_exception(key);
                init();
                return Block::RES_OK;
            }
        };
    } // namespace ndsp
} // namespace satdump