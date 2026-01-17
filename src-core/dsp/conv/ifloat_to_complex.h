#pragma once

#include "common/dsp/complex.h"
#include "dsp/block_simple.h"

namespace satdump
{
    namespace ndsp
    {
        class IFloatToComplexBlock : public BlockSimple<float, complex_t>
        {
        private:
            uint8_t state = 0;
            complex_t wip_c;

        public:
            uint32_t process(float *input, uint32_t nsamples, complex_t *output);

        public:
            IFloatToComplexBlock();
            ~IFloatToComplexBlock();

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