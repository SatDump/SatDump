#include "psk_demod.h"
#include "common/dsp/block.h"

namespace satdump
{
    namespace ndsp
    {
        PSKDemodHierBlock::PSKDemodHierBlock() : Block("psk_demod_cc", {}, {})
        {
            const_split_blk.add_output("main");
            const_split_blk.add_output("snr");
        }

        PSKDemodHierBlock::~PSKDemodHierBlock() {}
    } // namespace ndsp
} // namespace satdump