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

            agc_blk.set_cfg("reference", 0.6);
        }

        PSKDemodHierBlock::~PSKDemodHierBlock() {}
    } // namespace ndsp
} // namespace satdump