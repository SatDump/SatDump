#include "unpack_bits.h"
#include "common/codings/randomization.h"
#include <cstdint>

namespace satdump
{
    namespace ndsp
    {
        UnpackBitsBlock::UnpackBitsBlock() : BlockSimple<int8_t, int8_t>("unpack_bits_hh", {{"in", DSP_SAMPLE_TYPE_S8}}, {{"out", DSP_SAMPLE_TYPE_S8}}) { output_buffer_size_ratio = 8; }

        UnpackBitsBlock::~UnpackBitsBlock() {}

        uint32_t UnpackBitsBlock::process(int8_t *in, uint32_t nsamples, int8_t *out)
        {
            for (int i = 0; i < nsamples; i++)
            {
                for (int y = 7, y2 = 0; y >= 0; y--)
                {
                    out[i * 8 + y2] = (in[i] >> y) & 1;
                    y2++;
                }
            }

            return nsamples * 8;
        }
    } // namespace ndsp
} // namespace satdump