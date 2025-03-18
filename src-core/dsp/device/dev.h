#pragma once

#include "dsp/block.h"

// TODOREWORK DOCUMENT
namespace satdump
{
    namespace ndsp
    {
        struct DeviceInfo
        {
            std::string type;
            std::string name;
            nlohmann::json cfg;
        };

        class DeviceBlock : public Block
        {
        public:
            DeviceBlock(std::string id, std::vector<BlockIO> in = {}, std::vector<BlockIO> out = {}) : Block(id, in, out) {}
            virtual void drawUI() = 0;
        };
    }
}