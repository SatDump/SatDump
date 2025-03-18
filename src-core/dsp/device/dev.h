#pragma once

#include "dsp/block.h"

namespace satdump
{
    namespace ndsp
    {
        class DeviceBlock : public Block
        {
        public:
            DeviceBlock(std::string id, std::vector<BlockIO> in = {}, std::vector<BlockIO> out = {}) : Block(id, in, out) {}
            virtual void drawUI() = 0;
        };
    }
}