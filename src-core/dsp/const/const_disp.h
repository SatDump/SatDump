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

            nlohmann::json get_cfg()
            {
                nlohmann::json v;
                //    v["rate"] = rate;
                return v;
            }

            void set_cfg(nlohmann::json /*v*/)
            {
                //  rate = v["rate"];
            }
        };
    }
}