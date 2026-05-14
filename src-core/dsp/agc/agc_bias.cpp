#include "agc_bias.h"
#include "common/dsp/complex.h"

namespace satdump
{
    namespace ndsp
    {
        AGCBiasBlock::AGCBiasBlock() : BlockSimple<float, float>("agc_bias", {{"in", DSP_SAMPLE_TYPE_F32}}, {{"out", DSP_SAMPLE_TYPE_F32}}) {}

        AGCBiasBlock::~AGCBiasBlock() {}

        void AGCBiasBlock::init()
        {
            bias = 0;
            moving_avg = target;
        }

        uint32_t AGCBiasBlock::process(float *in, uint32_t nsamples, float *out)
        {
            for (uint32_t i = 0; i < nsamples; i++)
            {
                float sample = in[i];
                float gain;

                if (sample == 0)
                    return 0;
                sample -= bias;
                bias = bias * (1 - bias_pole) + sample * bias_pole;

                gain = target / moving_avg;
                moving_avg = moving_avg * (1 - gain_pole) + fabsf(sample) * gain_pole;

                sample *= gain;

                out[i] = sample;
            }

            return nsamples;
        }
    } // namespace ndsp
} // namespace satdump