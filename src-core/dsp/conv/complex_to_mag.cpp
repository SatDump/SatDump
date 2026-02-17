#include "complex_to_mag.h"

namespace satdump
{
    namespace ndsp
    {
        ComplexToMagBlock::ComplexToMagBlock()
            : BlockSimple<complex_t, float>(      //
                  "complex_to_mag_cf",            //
                  {{"in", DSP_SAMPLE_TYPE_CF32}}, //
                  {{"out", DSP_SAMPLE_TYPE_F32}})
        {
        }

        ComplexToMagBlock::~ComplexToMagBlock() {}

        uint32_t ComplexToMagBlock::process(complex_t *input, uint32_t nsamples, float *output)
        {
            volk_32fc_magnitude_32f(output, (lv_32fc_t *)input, nsamples);

            return nsamples;
        }
    } // namespace ndsp
} // namespace satdump
