#include "cadu_derand.h"
#include "common/codings/randomization.h"
#include <cstdint>

namespace satdump
{
    namespace ndsp
    {
        CADUDerandBlock::CADUDerandBlock() : BlockSimple<int8_t, int8_t>("cadu_derand_hh", {{"in", DSP_SAMPLE_TYPE_S8}}, {{"out", DSP_SAMPLE_TYPE_S8}}) {}

        CADUDerandBlock::~CADUDerandBlock() {}

        uint32_t CADUDerandBlock::process(int8_t *in, uint32_t nsamples, int8_t *out)
        {
            if (nsamples == 0)
                return 0;
            if (cadu_size - offset <= 0)
                return 0;
            if ((nsamples % cadu_size) != 0)
                return 0;

            for (int i = 0; i < nsamples; i += cadu_size)
            {
                derand_ccsds((uint8_t *)in + i + offset, cadu_size - offset);
                memcpy(out + i, in + i, cadu_size);
            }

            return nsamples;
        }
    } // namespace ndsp
} // namespace satdump