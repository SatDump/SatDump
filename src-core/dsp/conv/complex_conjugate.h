#pragma once

#include "dsp/block.h"
#include "dsp/block_helpers.h"
#include "dsp/block_simple.h"
#include <cstdint>

namespace satdump
{
    namespace ndsp
    {
        class ComplexConjugateBlock : public BlockSimple<complex_t, complex_t>
        {
        public:
            ComplexConjugateBlock();
            ~ComplexConjugateBlock();

            uint32_t process(complex_t *input, uint32_t nsamples, complex_t *output);

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
