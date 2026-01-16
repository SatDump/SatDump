#include "char_to_float.h"
#include "dsp/block.h"
#include <cstdint>

namespace satdump
{
    namespace ndsp
    {
        CharToFloatBlock::CharToFloatBlock() : BlockSimple("char_to_float_hf", {{"in", DSP_SAMPLE_TYPE_S8}}, {{"out", DSP_SAMPLE_TYPE_F32}}) {}

        CharToFloatBlock::~CharToFloatBlock() {}

        uint32_t CharToFloatBlock::process(int8_t *input, uint32_t nsamples, float *output)
        {
            volk_8i_s32f_convert_32f(output, input, scale, nsamples);

            return nsamples;
        }
    } // namespace ndsp
} // namespace satdump