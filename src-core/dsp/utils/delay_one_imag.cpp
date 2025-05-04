#include "delay_one_imag.h"

namespace satdump
{
    namespace ndsp
    {
        DelayOneImagBlock::DelayOneImagBlock() : satdump::ndsp::BlockSimple<complex_t, complex_t>("delay_one_imag_cc", {{"in", DSP_SAMPLE_TYPE_CF32}}, {{"out", DSP_SAMPLE_TYPE_CF32}}) {}

        DelayOneImagBlock::~DelayOneImagBlock() {}

        uint32_t DelayOneImagBlock::process(complex_t *input, uint32_t nsamples, complex_t *output)
        {
            for (uint32_t i = 0; i < nsamples; i++)
            {
                float imag = i == 0 ? lastSamp : input[i - 1].imag;
                float real = input[i].real;
                output[i] = complex_t(real, imag);
            }

            lastSamp = input[nsamples - 1].imag;

            return nsamples;
        }
    }; // namespace ndsp
} // namespace satdump
