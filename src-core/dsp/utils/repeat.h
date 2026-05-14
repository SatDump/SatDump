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
        class RepeatBlock : public Block
        {
        private:
            int interpolation = 2;

            bool work();

        public:
            RepeatBlock();
            ~RepeatBlock();

            void init() {}

            nlohmann::ordered_json get_cfg_list()
            {
                nlohmann::ordered_json p;
                add_param_simple(p, "interpolation", "int");
                return p;
            }

            nlohmann::json get_cfg(std::string key)
            {
                if (key == "interpolation")
                    return interpolation;
                else
                    throw satdump_exception(key);
            }

            cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                if (key == "interpolation")
                    interpolation = v;
                else
                    throw satdump_exception(key);
                return RES_OK;
            }
        };
    } // namespace ndsp
} // namespace satdump
