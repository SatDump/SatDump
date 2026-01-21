#pragma once

#include "common/dsp/complex.h"
#include "dsp/block_simple_multi.h"

namespace satdump
{
    namespace ndsp
    {
        class FloatToComplexBlock : public BlockSimpleMulti<float, complex_t, 2, 1>
        {
        public:
            void process(float **input, uint32_t *nsamples, complex_t **output, uint32_t *nsamples_out);

        public:
            FloatToComplexBlock();
            ~FloatToComplexBlock();

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