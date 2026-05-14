#pragma once

#include "dsp/block.h"
#include "dsp/block_simple.h"

namespace satdump
{
    namespace ndsp
    {
        class UnpackBitsBlock : public BlockSimple<int8_t, int8_t>
        {
        public:
            uint32_t process(int8_t *input, uint32_t nsamples, int8_t *output);

        public:
            UnpackBitsBlock();
            ~UnpackBitsBlock();

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