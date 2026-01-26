#pragma once

#include "dsp/block.h"
#include "dsp/block_helpers.h"
#include "dsp/block_simple_multi.h"
#include "nlohmann/json.hpp"
#include <cstdint>
#include <volk/volk.h>

namespace satdump
{
    namespace ndsp
    {
        template <typename T>
        class MultiplyBlock : public BlockSimpleMulti<T, T, 2, 1>
        {
        public:
            void process(T **input, uint32_t *nsamples, T **output, uint32_t *nsamples_out);

        public:
            MultiplyBlock();
            ~MultiplyBlock();

            void init() {}

            nlohmann::ordered_json get_cfg_list()
            {
                nlohmann::ordered_json p;
                return p;
            }

            nlohmann::json get_cfg(std::string key) { throw satdump_exception(key); }

            Block::cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                throw satdump_exception(key);
                return Block::RES_OK;
            }
        };
    } // namespace ndsp
} // namespace satdump
