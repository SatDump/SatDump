#include "audio_demod.h"
#include "common/dsp/block.h"

namespace satdump
{
    namespace ndsp
    {
        AudioDemodHierBlock::AudioDemodHierBlock() : Block("audio_demod_cc", {}, {}) {}

        AudioDemodHierBlock::~AudioDemodHierBlock() {}
    } // namespace ndsp
} // namespace satdump