#include "float_to_char.h"
#include "dsp/block.h"
#include <cstdint>

namespace satdump
{
    namespace ndsp
    {
        FloatToCharBlock::FloatToCharBlock() : BlockSimple("float_to_char_fh", {{"in", DSP_SAMPLE_TYPE_F32}}, {{"out", DSP_SAMPLE_TYPE_S8}}) {}

        FloatToCharBlock::~FloatToCharBlock() {}

        uint32_t FloatToCharBlock::process(float *input, uint32_t nsamples, int8_t *output)
        {
            volk_32f_s32f_convert_8i(output, input, scale, nsamples);

            return nsamples;
        }
    } // namespace ndsp
} // namespace satdump