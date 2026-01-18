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
            VCOBlock();
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
    } // namespace ndsp
} // namespace satdump