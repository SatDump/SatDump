#pragma once

#include "dsp/block.h"
#include "dsp/block_helpers.h"
#include "dsp/block_simple.h"
#include <cstdint>
#include <volk/volk.h>
#include <volk/volk_complex.h>

namespace satdump
{
    namespace ndsp
    {
        class ComplexToMagBlock : public BlockSimple<complex_t, float>
        {
        public:
            ComplexToMagBlock();
            ~ComplexToMagBlock();

            uint32_t process(complex_t *input, uint32_t nsamples, float *output);

            void init() {}

            nlohmann::ordered_json get_cfg_list()
            {
                nlohmann::ordered_json p;
                return p;
            }

            nlohmann::json get_cfg(std::string key) { throw satdump_exception(key); }

            cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                throw satdump_exception(key);
                return RES_OK;
            }
        };
    } // namespace ndsp
} // namespace satdump
