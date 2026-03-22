#include "clock_recovery_mm_fast.h"
#include "common/dsp/block.h"
#include "common/dsp/window/window.h"

#define BRANCHLESS_CLIP(x, clip) (0.5 * (std::abs(x + clip) - std::abs(x - clip)))

namespace satdump
{
    namespace ndsp
    {
        inline double hz_to_rad(double freq, double samplerate) { return 2.0 * M_PI * (freq / samplerate); }

        template <typename T>
        MMClockRecoveryFastBlock<T>::MMClockRecoveryFastBlock()
            : Block(std::is_same_v<T, complex_t> ? "fast_clock_recovery_mm_cc" : "fast_clock_recovery_mm_ff", {{"in", std::is_same_v<T, complex_t> ? DSP_SAMPLE_TYPE_CF32 : DSP_SAMPLE_TYPE_F32}},
                    {{"out", std::is_same_v<T, complex_t> ? DSP_SAMPLE_TYPE_CF32 : DSP_SAMPLE_TYPE_F32}})
        {
            // Buffer
            if (buffer == nullptr)
                volk_free(buffer); // TODOREWORK
#if MM_DO_BRANCH
            buffer = create_volk_buffer<T>(pfb.ntaps * 4);
#else
            buffer = dsp::create_volk_buffer<T>(1e6);
#endif
        }

        template <typename T>
        void MMClockRecoveryFastBlock<T>::init()
        {
            mu = p_mu;
            omega = p_omega;
            omega_gain = p_omegaGain;
            mu_gain = p_muGain;
            omega_relative_limit = p_omegaLimit;
            p_2T = 0;
            p_1T = 0;
            p_0T = 0;
            c_2T = 0;
            c_1T = 0;
            c_0T = 0;

            phase_error = 0;
            last_sample = 0;
            ouc = 0;
            inc = 0;

            // Omega setup
            omega_mid = omega;
            omega_limit = omega_relative_limit * omega;
        }

        template <typename T>
        MMClockRecoveryFastBlock<T>::~MMClockRecoveryFastBlock()
        {
            if (buffer == nullptr)
                volk_free(buffer);
        }

        template <typename T>
        bool MMClockRecoveryFastBlock<T>::work()
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
            memcpy(&buffer[ntaps - 1], ibuf, nsamples * sizeof(T));
            ouc = 0;

            for (; inc < nsamples && ouc < nsamples * 2 /* TODO THIS PROBABLY SUCKS!!!!*/;)
            {
                if constexpr (std::is_same_v<T, complex_t>)
                {
                    // Propagate delay
                    p_2T = p_1T;
                    p_1T = p_0T;
                    c_2T = c_1T;
                    c_1T = c_0T;
                }

                if constexpr (std::is_same_v<T, float>)
                {
                    // volk_32f_x2_dot_prod_32f(&sample, &buffer[inc], pfb.taps[imu], pfb.ntaps);
                    sample = buffer[inc] * (1.0 - mu) + buffer[inc + 1] * mu;

                    // Phase error
                    phase_error = (last_sample < 0 ? -1.0f : 1.0f) * sample - (sample < 0 ? -1.0f : 1.0f) * last_sample;
                    phase_error = dsp::branched_clip(phase_error, 1.0);
                    last_sample = sample;

                    // Write output sample
                    obuf[ouc++] = sample;
                }
                if constexpr (std::is_same_v<T, complex_t>)
                {
                    // volk_32fc_32f_dot_prod_32fc((lv_32fc_t *)&p_0T, (lv_32fc_t *)&buffer[inc], pfb.taps[imu], pfb.ntaps);
                    p_0T = buffer[inc] * (1.0 - mu) + buffer[inc + 1] * mu;

                    // Slice it
                    c_0T = complex_t(p_0T.real > 0.0f ? 1.0f : 0.0f, p_0T.imag > 0.0f ? 1.0f : 0.0f);

                    // Phase error
                    phase_error = (((p_0T - p_2T) * c_1T.conj()) - ((c_0T - c_2T) * p_1T.conj())).real;
                    phase_error = dsp::branched_clip(phase_error, 1.0);

                    // Write output
                    obuf[ouc++] = p_0T;
                }

                if (omega_upd_cnt++ == 4)
                {
                    omega_upd_cnt = 0;

                    // Adjust omega
                    omega = omega + omega_gain * phase_error;
                    omega = omega_mid + dsp::branched_clip((omega - omega_mid), omega_limit);
                }

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
            memmove(&buffer[0], &buffer[nsamples], ntaps * sizeof(T));

            oblk.size = ouc;
            outputs[0].fifo->wait_enqueue(oblk);
            inputs[0].fifo->free(iblk);

            return false;
        }

        template class MMClockRecoveryFastBlock<complex_t>;
        template class MMClockRecoveryFastBlock<float>;
    } // namespace ndsp
} // namespace satdump