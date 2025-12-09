#include "channel_model_simple.h"
#include "common/dsp/block.h"
#include "common/dsp/complex.h"

namespace satdump
{
    namespace ndsp
    {
        ChannelModelSimpleBlock::ChannelModelSimpleBlock()
            : BlockSimple<complex_t, complex_t>("channel_model_simple_cc", //
                                                {{"in", DSP_SAMPLE_TYPE_CF32}}, {{"out", DSP_SAMPLE_TYPE_CF32}})
        {
        }

        ChannelModelSimpleBlock::~ChannelModelSimpleBlock() {}

        uint32_t ChannelModelSimpleBlock::process(complex_t *in, uint32_t nsamples, complex_t *out)
        {
            // Frequency shift
            for (int i = 0; i < nsamples; i++)
            {
                // Mix input & VCO
                out[i] = in[i] * complex_t(cosf(-curr_phase), sinf(-curr_phase));

                // Increment phase
                curr_phase += curr_freq;

                // Wrap phase
                while (curr_phase > (2 * M_PI))
                    curr_phase -= 2 * M_PI;
                while (curr_phase < (-2 * M_PI))
                    curr_phase += 2 * M_PI;

                // Slowly ramp up freq toward target freq
                curr_freq = curr_freq * (1.0 - d_alpha) + dsp::hz_to_rad(-freq_shift, samplerate) * d_alpha;
            }

            // Attenuate signal down to wanted level
            double att = (-noise_level) - (signal_level - noise_level) - 26.5;
            complex_t attenuation_linear = complex_t(1.0 / std::pow(10.0f, (att / 20.0f)), 0.0f);
            volk_32fc_s32fc_multiply_32fc((lv_32fc_t *)out, (lv_32fc_t *)out, attenuation_linear, nsamples);

            // Add noise
            double noise_imp = 1;
            double noise_snr_linear = 1e3 * pow(10, noise_level / 10);
            complex_t noise_target_level = complex_t(sqrt((noise_imp * noise_snr_linear) / 2));

            for (size_t i = 0; i < nsamples; i++)
            {
                complex_t ns = complex_t(sqrt((noise_imp * noise_snr_linear) / 2)) * complex_t(d_rng.gasdev(), d_rng.gasdev());
                out[i] = out[i] + ns;
            }

            return nsamples;
        }
    } // namespace ndsp
} // namespace satdump
