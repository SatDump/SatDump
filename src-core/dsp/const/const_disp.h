#pragma once

#include "dsp/block.h"
#include "common/dsp/complex.h"
#include "common/widgets/constellation.h"

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

            void set_cfg(std::string key, nlohmann::json v)
            {
                // if (key == "max_gain")
                //     p_max_gain = v;
                // else
                throw satdump_exception(key);
            }
        };
    }
}