#pragma once

#include "common/dsp/complex.h"
#include "dsp/block.h"
#include "dsp/block_simple.h"
#include <cstdint>

namespace satdump
{
    namespace ndsp
    {
        class VCOBlock : public BlockSimple<float, complex_t>
        {
        private:
            float d_k = 1;
            float d_amp = 1;
            float d_phase = 0;

        public:
            VCOBlock(std::string id = "vco_fc");
            ~VCOBlock();

            uint32_t process(float *input, uint32_t nsamples, complex_t *output);

            void init() {}

            nlohmann::ordered_json get_cfg_list()
            {
                nlohmann::ordered_json p;
                add_param_simple(p, "k", "float");
                add_param_simple(p, "amp", "float");
                return p;
            }

            nlohmann::json get_cfg(std::string key)
            {
                if (key == "k")
                    return d_k;
                else if (key == "amp")
                    return d_amp;
                else
                    throw satdump_exception(key);
            }

            cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                if (key == "k")
                    d_k = v;
                else if (key == "amp")
                    d_amp = v;
                else
                    throw satdump_exception(key);
                return RES_OK;
            }
        };

        class VCOBlockDeviation : public VCOBlock
        {
        private:
            double samplerate = 48e3;
            double deviation = 2e3;

        public:
            VCOBlockDeviation() : VCOBlock("vco_dev_fc") {}

            nlohmann::ordered_json get_cfg_list()
            {
                nlohmann::ordered_json p;
                add_param_simple(p, "samplerate", "float");
                add_param_simple(p, "deviation", "float");
                add_param_simple(p, "amp", "float");
                return p;
            }

            nlohmann::json get_cfg(std::string key)
            {
                if (key == "samplerate")
                    return samplerate;
                else if (key == "deviation")
                    return deviation;
                else if (key == "amp")
                    return VCOBlock::get_cfg(key);
                else
                    throw satdump_exception(key);
            }

            cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                if (key == "samplerate")
                {
                    samplerate = v;
                    VCOBlock::set_cfg("k", (2 * M_PI * deviation) / samplerate);
                }
                else if (key == "deviation")
                {
                    deviation = v;
                    VCOBlock::set_cfg("k", (2 * M_PI * deviation) / samplerate);
                }
                else if (key == "amp")
                    return VCOBlock::set_cfg(key, v);
                else
                    throw satdump_exception(key);
                return RES_OK;
            }
        };
    } // namespace ndsp
} // namespace satdump