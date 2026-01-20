#pragma once

#include "common/dsp/complex.h"
#include "dsp/block_simple_multi.h"

namespace satdump
{
    namespace ndsp
    {
        class ComplexToFloatBlock : public BlockSimpleMulti<complex_t, float, 1, 2>
        {
        public:
            void process(complex_t **input, uint32_t *nsamples, float **output, uint32_t *nsamples_out);

        public:
            ComplexToFloatBlock();
            ~ComplexToFloatBlock();

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