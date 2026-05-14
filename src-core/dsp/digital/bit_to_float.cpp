#include "bit_to_float.h"
#include "common/dsp/complex.h"
#include "dsp/block.h"
#include <cstdint>
#include <volk/volk.h>

namespace satdump
{
    namespace ndsp
    {
        BitToFloatBlock::BitToFloatBlock() : BlockSimple<int8_t, float>("bit_to_float_hf", {{"in", DSP_SAMPLE_TYPE_S8}}, {{"out", DSP_SAMPLE_TYPE_F32}}) {}

        BitToFloatBlock::~BitToFloatBlock() {}

        uint32_t BitToFloatBlock::process(int8_t *in, uint32_t nsamples, float *out)
        {
            for (uint32_t i = 0; i < nsamples; i++)
                out[i] = in[i] ? 1 : -1;
            return nsamples;
        }
    } // namespace ndsp
} // namespace satdump