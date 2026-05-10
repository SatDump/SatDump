#pragma once

#include "dsp/block.h"
#include "dsp/block_helpers.h"
#include "dsp/block_simple.h"
#include "dsp/block_simple_multi.h"
#include "nlohmann/json.hpp"
#include <cstdint>
#include <volk/volk.h>

namespace satdump
{
    namespace ndsp
    {
        template <typename T>
        class MultiplyConstBlock : public BlockSimple<T, T>
        {
        private:
            float mult_const = 1;

        public:
            uint32_t process(T *input, uint32_t nsamples, T *output);

        public:
            MultiplyConstBlock();
            ~MultiplyConstBlock();

            void init() {}

            nlohmann::ordered_json get_cfg_list()
            {
                nlohmann::ordered_json p;
                add_param_simple(p, "const", "float", "Constant");
                return p;
            }

            nlohmann::json get_cfg(std::string key)
            {
                if (key == "const")
                    return mult_const;
                else
                    throw satdump_exception(key);
            }

            Block::cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                if (key == "const")
                    mult_const = v;
                else
                    throw satdump_exception(key);
                return Block::RES_OK;
            }
        };
    } // namespace ndsp
} // namespace satdump
