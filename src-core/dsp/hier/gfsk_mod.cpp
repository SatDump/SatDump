#include "gfsk_mod.h"
#include "common/dsp/block.h"

namespace satdump
{
    namespace ndsp
    {
        GFSKModHierBlock::GFSKModHierBlock() : Block("gfsk_mod_cc", {}, {}) {}

        GFSKModHierBlock::~GFSKModHierBlock() {}
    } // namespace ndsp
} // namespace satdump