#include "uchar_to_float.h"
#include "dsp/block.h"
#include <cstdint>

namespace satdump
{
    namespace ndsp
    {
        UCharToFloatBlock::UCharToFloatBlock() : BlockSimple("uchar_to_float_bf", {{"in", DSP_SAMPLE_TYPE_U8}}, {{"out", DSP_SAMPLE_TYPE_F32}}) {}

        UCharToFloatBlock::~UCharToFloatBlock() {}

        uint32_t UCharToFloatBlock::process(uint8_t *input, uint32_t nsamples, float *output)
        {
            for (int i = 0; i < nsamples; i++)
                output[i] = input[i] * scale;

            return nsamples;
        }
    } // namespace ndsp
} // namespace satdump