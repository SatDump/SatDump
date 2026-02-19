#include "psk_demod.h"
#include "common/dsp/block.h"

namespace satdump
{
    namespace ndsp
    {
        PSKDemodHierBlock::PSKDemodHierBlock() : Block("psk_demod_cc", {}, {}) {}

        PSKDemodHierBlock::~PSKDemodHierBlock() {}
    } // namespace ndsp
} // namespace satdump