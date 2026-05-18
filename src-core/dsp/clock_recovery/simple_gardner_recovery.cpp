#include "simple_gardner_recovery.h"

namespace satdump
{
    namespace ndsp
    {
        SimpleGardnerRecoveryBlock::SimpleGardnerRecoveryBlock() : BlockSimple<complex_t, complex_t>("simple_gardner_recovery_c", {{"in", DSP_SAMPLE_TYPE_CF32}}, {{"out", DSP_SAMPLE_TYPE_CF32}}) {}

        SimpleGardnerRecoveryBlock::~SimpleGardnerRecoveryBlock() {}

        void SimpleGardnerRecoveryBlock::init() {}

        bool SimpleGardnerRecoveryBlock::clock_recovery(complex_t sample, complex_t *sym)
        {
            bool fired = 0;

            // Down to next half-symbol
            phase -= 1.0f;

            if (phase <= 0.0f)
            {
                // Interpolation either midpoint or
                float frac = 1.0f + phase;
                complex_t samp = last_sample + (sample - last_sample) * frac;

                if (is_mid)
                {
                    // Midpoint sample (between two symbols)
                    mid = samp;
                    is_mid = false;
                    phase += period / 2.0f;
                }
                else
                {
                    // On-time symbol sample
                    complex_t curr = samp;

                    complex_t diff = curr - prev_sym;
                    float err = diff.real * mid.real + diff.imag * mid.imag;

                    // Advance to next midpoint with error
                    phase += period / 2.0f - alpha * err;

                    prev_sym = curr;
                    is_mid = true;

                    *sym = curr;
                    fired = 1;
                }
            }

            last_sample = sample;

            return fired;
        }

        uint32_t SimpleGardnerRecoveryBlock::process(complex_t *in, uint32_t nsamples, complex_t *out)
        {
            uint32_t oo = 0;
            complex_t sym;
            for (uint32_t i = 0; i < nsamples; i++)
                if (clock_recovery(in[i], &sym))
                    out[oo++] = sym;
            return oo;
        }
    } // namespace ndsp
} // namespace satdump