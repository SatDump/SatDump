#pragma once

#include "common/dsp/complex.h"
#include "common/widgets/constellation.h"
#include "dsp/block.h"

namespace satdump
{
    namespace ndsp
    {
        class ConstellationDisplayBlock : public Block
        {
        public:
            widgets::ConstellationViewer constel;

        public:
            ConstellationDisplayBlock();
            ~ConstellationDisplayBlock();

            bool work();

            nlohmann::json get_cfg(std::string key)
            {
                // if (key == "max_gain")
                //     return p_max_gain;
                // else
                throw satdump_exception(key);
            }

            cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                // if (key == "max_gain")
                //     p_max_gain = v;
                // else
                throw satdump_exception(key);
                return RES_OK;
            }
        };
    } // namespace ndsp
} // namespace satdump