#include "short_to_float.h"
#include "dsp/block.h"
#include <cstdint>

namespace satdump
{
    namespace ndsp
    {
        ShortToFloatBlock::ShortToFloatBlock() : BlockSimple("short_to_float_hf", {{"in", DSP_SAMPLE_TYPE_S16}}, {{"out", DSP_SAMPLE_TYPE_F32}}) {}

        ShortToFloatBlock::~ShortToFloatBlock() {}

        uint32_t ShortToFloatBlock::process(int16_t *input, uint32_t nsamples, float *output)
        {
            volk_16i_s32f_convert_32f(output, input, scale, nsamples);

            return nsamples;
        }
    } // namespace ndsp
} // namespace satdump