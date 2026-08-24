#include "clock_recovery_gardner.h"
#include "common/dsp/block.h"
#include "common/dsp/window/window.h"

#define BRANCHLESS_CLIP(x, clip) (0.5 * (std::abs(x + clip) - std::abs(x - clip)))

namespace satdump
{
    namespace ndsp
    {
        inline double hz_to_rad(double freq, double samplerate) { return 2.0 * M_PI * (freq / samplerate); }

        template <typename T>
        GardnerClockRecoveryBlock<T>::GardnerClockRecoveryBlock()
            : Block(std::is_same_v<T, complex_t> ? "clock_recovery_gardner_cc" : "clock_recovery_gardner_ff", {{"in", std::is_same_v<T, complex_t> ? DSP_SAMPLE_TYPE_CF32 : DSP_SAMPLE_TYPE_F32}},
                    {{"out", std::is_same_v<T, complex_t> ? DSP_SAMPLE_TYPE_CF32 : DSP_SAMPLE_TYPE_F32}})
        {
            // Buffer
            if (buffer == nullptr)
                volk_free(buffer); // TODOREWORK

            buffer = dsp::create_volk_buffer<T>(1e6);
        }

        template <typename T>
        void GardnerClockRecoveryBlock<T>::init()
        {
            mu = p_mu;
            omega = p_omega;
            omega_gain = p_omegaGain;
            mu_gain = p_muGain;
            omega_relative_limit = p_omegaLimit;
            sample = 0;
            zc_sample = 0;
            last_sample = 0;

            phase_error = 0;
            last_sample = 0;
            ouc = 0;
            inc = 0;

            // Omega setup
            omega_mid = omega;
            omega_limit = omega_relative_limit * omega;

            // Init interpolator
            pfb.init(dsp::windowed_sinc(p_nfilt * p_ntaps, hz_to_rad(0.5 / (double)p_nfilt, 1.0), dsp::window::nuttall, p_nfilt),
                     p_nfilt); // TODOREWORK do this in main loop? TODODSP

            bufs = 20;
        }

        template <typename T>
        GardnerClockRecoveryBlock<T>::~GardnerClockRecoveryBlock()
        {
            if (buffer == nullptr)
                volk_free(buffer);
        }

        template <typename T>
        bool GardnerClockRecoveryBlock<T>::work()
        {
            DSPBuffer iblk = inputs[0].fifo->wait_dequeue();

            if (iblk.isTerminator())
            {
                if (iblk.terminatorShouldPropagate())
                    outputs[0].fifo->wait_enqueue(outputs[0].fifo->newBufferTerminator());
                inputs[0].fifo->free(iblk);
                return true;
            }

            if (needs_reinit)
            {
                needs_reinit = false;
                init();
            }

            DSPBuffer oblk = outputs[0].fifo->newBufferSamples(iblk.max_size, sizeof(T));
            T *ibuf = iblk.getSamples<T>();
            T *obuf = oblk.getSamples<T>();

            int nsamples = iblk.size;

            // Copy NTAPS samples in the buffer from input, as that's required for the last samples
            memcpy(&buffer[pfb.ntaps - 1 + bufs], ibuf, nsamples * sizeof(T));

            ouc = 0;

            for (; inc < nsamples && ouc < nsamples * 2 /* TODO THIS PROBABLY SUCKS!!!!*/;)
            {
                // Compute Zero-Crossing sample
                float muz = mu - (omega / 2.0);
                int offzc = floor(omega / 2.0);
                float mupos = fmod(muz + offzc, 1.0);
                if (mupos < 0)
                {
                    mupos = 1 + mupos;
                    offzc += 1;
                }
                int imuz = (int)rint(mupos * pfb.nfilt);
                if (imuz < 0) // If we're out of bounds, clamp
                    imuz = 0;
                if (imuz >= pfb.nfilt)
                    imuz = pfb.nfilt - 1;

                // Compute output
                int imu = (int)rint(mu * pfb.nfilt);
                if (imu < 0) // If we're out of bounds, clamp
                    imu = 0;
                if (imu >= pfb.nfilt)
                    imu = pfb.nfilt - 1;

                if constexpr (std::is_same_v<T, float>)
                {
                    volk_32f_x2_dot_prod_32f(&zc_sample, &buffer[inc - offzc + bufs], pfb.taps[imuz], pfb.ntaps);

                    volk_32f_x2_dot_prod_32f(&sample, &buffer[inc + bufs], pfb.taps[imu], pfb.ntaps);

                    // Phase error
                    phase_error = zc_sample * (last_sample - sample);

                    phase_error = dsp::branched_clip(phase_error, 1.0);
                    last_sample = sample;

                    // Write output sample
                    obuf[ouc++] = sample;
                }
                if constexpr (std::is_same_v<T, complex_t>)
                {
                    volk_32fc_32f_dot_prod_32fc((lv_32fc_t *)&zc_sample, (lv_32fc_t *)&buffer[inc - offzc + bufs], pfb.taps[imuz], pfb.ntaps);

                    volk_32fc_32f_dot_prod_32fc((lv_32fc_t *)&sample, (lv_32fc_t *)&buffer[inc + bufs], pfb.taps[imu], pfb.ntaps);

                    // Phase error
                    phase_error = zc_sample.real * (last_sample.real - sample.real) + //
                                  zc_sample.imag * (last_sample.imag - sample.imag);

                    phase_error = dsp::branched_clip(phase_error, 1.0);
                    last_sample = sample;

                    // Write output
                    obuf[ouc++] = sample;
                }

                // Adjust omega
                omega = omega + omega_gain * phase_error;
                omega = omega_mid + dsp::branched_clip((omega - omega_mid), omega_limit);

                // Adjust phase
                mu = mu + omega + mu_gain * phase_error;
                inc += int(floor(mu));
                mu -= floor(mu);
                if (inc < 0)
                    inc = 0;
            }

            inc -= nsamples;

            if (inc < 0)
                inc = 0;

            // We need some history for the next run, so copy it over into our buffer
            memmove(&buffer[0], &buffer[nsamples], (pfb.ntaps + bufs) * sizeof(T));

            oblk.size = ouc;
            outputs[0].fifo->wait_enqueue(oblk);
            inputs[0].fifo->free(iblk);

            return false;
        }

        template class GardnerClockRecoveryBlock<complex_t>;
        template class GardnerClockRecoveryBlock<float>;
    } // namespace ndsp
} // namespace satdump