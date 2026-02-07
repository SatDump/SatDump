#pragma once

/**
 * @file complex_to_imag.h
 */

#include "common/dsp/complex.h"
#include "dsp/block_simple.h"

namespace satdump
{
    namespace ndsp
    {
        class ComplexToRealBlock : public BlockSimple<complex_t, float>
        {
        public:
            ComplexToRealBlock();
            ~ComplexToRealBlock();

            uint32_t process(complex_t *input, uint32_t nsamples, float *output);

            void init() {}

            nlohmann::json get_cfg(std::string key) { throw satdump_exception(key); }

            Block::cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                throw satdump_exception(key);
                return Block::RES_OK;
            }
        };
    } // namespace ndsp

} // namespace satdump
