#include "psk_demod_superfast.h"
#include "common/dsp/block.h"

namespace satdump
{
    namespace ndsp
    {
        PSKDemodSuperFastHierBlock::PSKDemodSuperFastHierBlock() : Block("psk_demod_superfast_cc", {}, {})
        {
            const_split_blk.add_output("main");
            const_split_blk.add_output("snr");

            agc_blk.set_cfg("reference", 0.6);
        }

        PSKDemodSuperFastHierBlock::~PSKDemodSuperFastHierBlock() {}
    } // namespace ndsp
} // namespace satdump