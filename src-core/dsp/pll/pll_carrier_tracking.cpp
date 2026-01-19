#include "pll_carrier_tracking.h"
#include "common/dsp/utils/fast_trig.h"

#define M_TWOPI (2 * M_PI)

namespace satdump
{
    namespace ndsp
    {
        inline float branchless_clip(float x, float clip) { return 0.5 * (std::abs(x + clip) - std::abs(x - clip)); }

        PLLCarrierTrackingBlock::PLLCarrierTrackingBlock() : BlockSimple("pll_carrier_tracking_cc", {{"in", DSP_SAMPLE_TYPE_CF32}}, {{"out", DSP_SAMPLE_TYPE_CF32}}) {}

        PLLCarrierTrackingBlock::~PLLCarrierTrackingBlock() {}

        uint32_t PLLCarrierTrackingBlock::process(complex_t *input, uint32_t nsamples, complex_t *output)
        {
            float phase_error = 0;  // Phase error
            complex_t vcoValue = 0; // Frequency offset

            for (uint32_t i = 0; i < nsamples; i++)
            {
                // Generate VCO
                vcoValue = complex_t(dsp::fast_cos(d_phase), -dsp::fast_sin(d_phase));

                // Mix with input
                output[i] = input[i] * vcoValue;

                // Compute phase error and clip it
                phase_error = dsp::fast_atan2f(input[i].imag, input[i].real) - d_phase;
                while (phase_error < -M_PI)
                    phase_error += M_TWOPI;
                while (phase_error > M_PI)
                    phase_error -= M_TWOPI;

                // Get new phase and freq, then wrap it
                d_freq = d_freq + d_beta * phase_error;
                if (d_freq > d_max_freq)
                    d_freq = d_max_freq;
                else if (d_freq < d_min_freq)
                    d_freq = d_min_freq;

                // Get new phase
                d_phase = d_phase + d_freq + d_alpha * phase_error;
                while (d_phase < -M_PI)
                    d_phase += M_TWOPI;
                while (d_phase > M_PI)
                    d_phase -= M_TWOPI;
            }

            return nsamples;
        }
    } // namespace ndsp
} // namespace satdump