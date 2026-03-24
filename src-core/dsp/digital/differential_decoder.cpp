#include "differential_decoder.h"
#include "dsp/block.h"
#include <cstdint>
#include <volk/volk.h>

namespace satdump
{
    namespace ndsp
    {
        DifferentialDecoderBlock::DifferentialDecoderBlock() : BlockSimple<int8_t, int8_t>("differential_decoder_hh", {{"in", DSP_SAMPLE_TYPE_S8}}, {{"out", DSP_SAMPLE_TYPE_S8}}) {}

        DifferentialDecoderBlock::~DifferentialDecoderBlock() {}

        uint32_t DifferentialDecoderBlock::process(int8_t *in, uint32_t nsamples, int8_t *out)
        {
            nrzm.decode_bits((uint8_t *)in, nsamples);
            memcpy(out, in, nsamples);

            return nsamples;
        }
    } // namespace ndsp
} // namespace satdump