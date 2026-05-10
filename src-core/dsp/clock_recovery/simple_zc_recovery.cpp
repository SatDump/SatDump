#include "simple_zc_recovery.h"
#include "common/dsp/complex.h"

namespace satdump
{
    namespace ndsp
    {
        SimpleZeroCrossingRecoveryBlock::SimpleZeroCrossingRecoveryBlock() : BlockSimple<float, float>("simple_zc_recovery", {{"in", DSP_SAMPLE_TYPE_F32}}, {{"out", DSP_SAMPLE_TYPE_F32}}) {}

        SimpleZeroCrossingRecoveryBlock::~SimpleZeroCrossingRecoveryBlock() {}

        void SimpleZeroCrossingRecoveryBlock::init() {}

        bool SimpleZeroCrossingRecoveryBlock::clock_recovery(float sample, float *sym)
        {
            bool fired = 0;

            // Zero-crossing detection
            if ((prev < 0.0f && sample >= 0.0f) || (prev >= 0.0f && sample < 0.0f))
            {
                float frac = prev / (prev - sample);

                // Phase error
                float expected = period / 2.0f;
                float actual = phase + frac;
                float err = actual - expected;

                phase -= err * alpha;
            }

            // Advance phase
            phase += 1.0f;

            // Generate (interpolate) output symbol
            if (phase >= period)
            {
                phase -= period;

                float frac = phase;
                *sym = prev + frac * (sample - prev);
                fired = 1;
            }

            prev = sample;
            return fired;
        }

        uint32_t SimpleZeroCrossingRecoveryBlock::process(float *in, uint32_t nsamples, float *out)
        {
            uint32_t oo = 0;
            float sym;
            for (uint32_t i = 0; i < nsamples; i++)
                if (clock_recovery(in[i], &sym))
                    out[oo++] = sym;
            return oo;
        }
    } // namespace ndsp
} // namespace satdump