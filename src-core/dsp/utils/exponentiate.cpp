#include "exponentiate.h"

namespace satdump
{
    namespace ndsp
    {
        ExponentiateBlock::ExponentiateBlock()
            : BlockSimple<complex_t, complex_t>(  //
                  "exponantiate_cc",              //
                  {{"in", DSP_SAMPLE_TYPE_CF32}}, //
                  {{"out", DSP_SAMPLE_TYPE_CF32}})
        {
        }

        ExponentiateBlock::~ExponentiateBlock() {}

        uint32_t ExponentiateBlock::process(complex_t *input, uint32_t nsamples, complex_t *output)
        {
            if (needs_reinit)
            {
                needs_reinit = false;
                init();
            }
            volk_32fc_x2_multiply_32fc((lv_32fc_t *)output, (lv_32fc_t *)input, (lv_32fc_t *)input, nsamples);
            for (uint32_t i = 2; i < d_expo; i++)
            {
                volk_32fc_x2_multiply_32fc((lv_32fc_t *)output, (lv_32fc_t *)output, (lv_32fc_t *)input, nsamples);
            }
            return nsamples;
        }
    } // namespace ndsp
} // namespace satdump
