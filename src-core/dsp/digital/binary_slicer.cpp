#include "binary_slicer.h"
#include "common/dsp/complex.h"
#include "dsp/block.h"
#include <cstdint>
#include <volk/volk.h>

namespace satdump
{
    namespace ndsp
    {
        BinarySlicerBlock::BinarySlicerBlock() : BlockSimple<float, int8_t>("binary_slicer_fh", {{"in", DSP_SAMPLE_TYPE_F32}}, {{"out", DSP_SAMPLE_TYPE_S8}}) {}

        BinarySlicerBlock::~BinarySlicerBlock() {}

        uint32_t BinarySlicerBlock::process(float *in, uint32_t nsamples, int8_t *out)
        {
            volk_32f_binary_slicer_8i((int8_t *)out, in, nsamples);

            return nsamples;
        }
    } // namespace ndsp
} // namespace satdump