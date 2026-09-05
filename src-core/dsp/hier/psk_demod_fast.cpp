#include "psk_demod_fast.h"
#include "common/dsp/block.h"

namespace satdump
{
    namespace ndsp
    {
        PSKDemodFastHierBlock::PSKDemodFastHierBlock() : Block("psk_demod_fast_cc", {}, {})
        {
            const_split_blk.add_output("main");
            const_split_blk.add_output("snr");

            agc_blk.set_cfg("reference", 0.6);
        }

        PSKDemodFastHierBlock::~PSKDemodFastHierBlock() {}
    } // namespace ndsp
} // namespace satdump