#include "complex_to_mag_squared.h"

namespace satdump
{
    namespace ndsp
    {
        ComplexToMagSquaredBlock::ComplexToMagSquaredBlock()
            : BlockSimple<complex_t, float>(      //
                  "complex_to_mag_squared_cf",    //
                  {{"in", DSP_SAMPLE_TYPE_CF32}}, //
                  {{"out", DSP_SAMPLE_TYPE_F32}})
        {
        }

        ComplexToMagSquaredBlock::~ComplexToMagSquaredBlock() {}

        uint32_t ComplexToMagSquaredBlock::process(complex_t *input, uint32_t nsamples, float *output)
        {
            volk_32fc_magnitude_squared_32f(output, (lv_32fc_t *)input, nsamples);

            return nsamples;
        }
    } // namespace ndsp
} // namespace satdump
