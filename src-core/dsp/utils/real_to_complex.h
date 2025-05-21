#pragma once

/**
 * @file real_to_complex.h
 */

#include "common/dsp/complex.h"
#include "dsp/block_simple.h"

namespace satdump
{
    namespace ndsp
    {
        class RealToComplexBlock : public BlockSimple<float, complex_t>
        {
        public:
            RealToComplexBlock();
            ~RealToComplexBlock();

            uint32_t process(float *input, uint32_t nsamples, complex_t *output);

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
