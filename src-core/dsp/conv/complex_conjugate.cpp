#include "complex_conjugate.h"

namespace satdump
{
    namespace ndsp
    {
        ComplexConjugateBlock::ComplexConjugateBlock()
            : BlockSimple<complex_t, complex_t>(  //
                  "complex_conjugate_cc",         //
                  {{"in", DSP_SAMPLE_TYPE_CF32}}, //
                  {{"out", DSP_SAMPLE_TYPE_CF32}})
        {
        }

        ComplexConjugateBlock::~ComplexConjugateBlock() {}

        uint32_t ComplexConjugateBlock::process(complex_t *input, uint32_t nsamples, complex_t *output)
        {
            volk_32fc_conjugate_32fc((lv_32fc_t *)output, (const lv_32fc_t *)input, nsamples);

            return nsamples;
        }
    } // namespace ndsp
} // namespace satdump
