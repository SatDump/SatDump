#include "bits_repack.h"
#include "common/codings/randomization.h"
#include <cstdint>

namespace satdump
{
    namespace ndsp
    {
        BitsRepackBlock::BitsRepackBlock() : BlockSimple<int8_t, int8_t>("bits_repack_hh", {{"in", DSP_SAMPLE_TYPE_S8}}, {{"out", DSP_SAMPLE_TYPE_S8}}) {}

        BitsRepackBlock::~BitsRepackBlock() {}

        uint32_t BitsRepackBlock::process(int8_t *in, uint32_t nsamples, int8_t *out)
        {
            if (nsamples == 0)
                return 0;

            int no = 0;
            for (int i = 0; i < nsamples; i++)
            {
                shifter = shifter << 1 | in[i];
                in_shifter++;
                if (in_shifter >= 8)
                {
                    out[no++] = shifter;
                    in_shifter = 0;
                }
            }

            return no;
        }
    } // namespace ndsp
} // namespace satdump