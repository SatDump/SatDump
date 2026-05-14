#pragma once

#include "common/dsp/block.h"
#include "common/dsp/complex.h"
#include "common/dsp/filter/firdes.h"
#include "dsp/block.h"
#include <volk/volk.h>

namespace satdump
{
    namespace ndsp
    {
        template <typename T>
        class ZeroFillBlock : public Block
        {
        private:
            int zero_buffer_size = 64;
            bool work();

        public:
            ZeroFillBlock();
            ~ZeroFillBlock();

            void init() {}

            nlohmann::ordered_json get_cfg_list()
            {
                nlohmann::ordered_json p;
                add_param_simple(p, "zero_buffer_size", "int");
                return p;
            }

            nlohmann::json get_cfg(std::string key)
            {
                if (key == "zero_buffer_size")
                    return zero_buffer_size;
                else
                    throw satdump_exception(key);
            }

            cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                if (key == "zero_buffer_size")
                    zero_buffer_size = v;
                else
                    throw satdump_exception(key);
                return RES_OK;
            }
        };
    } // namespace ndsp
} // namespace satdump
