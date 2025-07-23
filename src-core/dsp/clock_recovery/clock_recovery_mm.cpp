#include "clock_recovery_mm.h"
#include "common/dsp/block.h"
#include "common/dsp/window/window.h"

#define BRANCHLESS_CLIP(x, clip) (0.5 * (std::abs(x + clip) - std::abs(x - clip)))

namespace satdump
{
    namespace ndsp
    {
        inline double hz_to_rad(double freq, double samplerate) { return 2.0 * M_PI * (freq / samplerate); }

        template <typename T>
        MMClockRecoveryBlock<T>::MMClockRecoveryBlock()
            : Block(std::is_same_v<T, complex_t> ? "clock_recovery_mm_cc" : "clock_recovery_mm_ff", {{"in", std::is_same_v<T, complex_t> ? DSP_SAMPLE_TYPE_CF32 : DSP_SAMPLE_TYPE_F32}},
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
        void MMClockRecoveryBlock<T>::init()
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

            // Init interpolator
            pfb.init(dsp::windowed_sinc(p_nfilt * p_ntaps, hz_to_rad(0.5 / (double)p_nfilt, 1.0), dsp::window::nuttall, p_nfilt),
                     p_nfilt); // TODOREWORK do this in main loop? TODODSP
        }

        template <typename T>
        MMClockRecoveryBlock<T>::~MMClockRecoveryBlock()
        {
            if (buffer == nullptr)
                volk_free(buffer);
        }

        template <typename T>
        bool MMClockRecoveryBlock<T>::work()
        {
            DSPBuffer iblk;
            inputs[0].fifo->wait_dequeue(iblk);

            if (iblk.isTerminator())
            {
                if (iblk.terminatorShouldPropagate())
                    outputs[0].fifo->wait_enqueue(DSPBuffer::newBufferTerminator());
                iblk.free();
                return true;
            }

            if (needs_reinit)
            {
                needs_reinit = false;
                init();
            }

            DSPBuffer oblk = DSPBuffer::newBufferSamples<T>(iblk.max_size);
            T *ibuf = iblk.getSamples<T>();
            T *obuf = oblk.getSamples<T>();

            int nsamples = iblk.size;

            // Copy NTAPS samples in the buffer from input, as that's required for the last samples
#if MM_DO_BRANCH
            memcpy(&buffer[pfb.ntaps - 1], Block<T, T>::input_stream->readBuf, (pfb.ntaps - 1) * sizeof(T));
#else
            memcpy(&buffer[pfb.ntaps - 1], ibuf, nsamples * sizeof(T));
#endif
            ouc = 0;

            for (; inc < nsamples && ouc < nsamples * 2 /* TODO THIS PROBABLY SUCKS!!!!*/;)
            {
                //  if constexpr (std::is_same_v<T, complex_t>)
                {
                    // Propagate delay
                    p_2T = p_1T;
                    p_1T = p_0T;
                    c_2T = c_1T;
                    c_1T = c_0T;
                }

                // Compute output
                int imu = (int)rint(mu * pfb.nfilt);
                if (imu < 0) // If we're out of bounds, clamp
                    imu = 0;
                if (imu >= pfb.nfilt)
                    imu = pfb.nfilt - 1;

                if constexpr (std::is_same_v<T, float>)
                {
#if MM_DO_BRANCH
                    if (inc < (pfb.ntaps - 1))
                        volk_32f_x2_dot_prod_32f(&sample, &buffer[inc], pfb.taps[imu], pfb.ntaps);
                    else
                        volk_32f_x2_dot_prod_32f(&sample, &Block<T, T>::input_stream->readBuf[inc - (pfb.ntaps - 1)], pfb.taps[imu], pfb.ntaps);
#else
                    volk_32f_x2_dot_prod_32f(&sample, &buffer[inc], pfb.taps[imu], pfb.ntaps);
#endif

                    // Phase error
                    phase_error = (last_sample < 0 ? -1.0f : 1.0f) * sample - (sample < 0 ? -1.0f : 1.0f) * last_sample;
                    phase_error = dsp::branched_clip(phase_error, 1.0);
                    last_sample = sample;

                    // Write output sample
                    obuf[ouc++] = sample;
                }
                if constexpr (std::is_same_v<T, complex_t>)
                {
#if MM_DO_BRANCH
                    if (inc < (pfb.ntaps - 1))
                        volk_32fc_32f_dot_prod_32fc((lv_32fc_t *)&p_0T, (lv_32fc_t *)&buffer[inc], pfb.taps[imu], pfb.ntaps);
                    else
                        volk_32fc_32f_dot_prod_32fc((lv_32fc_t *)&p_0T, (lv_32fc_t *)&Block<T, T>::input_stream->readBuf[inc - (pfb.ntaps - 1)], pfb.taps[imu], pfb.ntaps);
#else
                    volk_32fc_32f_dot_prod_32fc((lv_32fc_t *)&p_0T, (lv_32fc_t *)&buffer[inc], pfb.taps[imu], pfb.ntaps);
#endif

                    // Slice it
                    c_0T = complex_t(p_0T.real > 0.0f ? 1.0f : 0.0f, p_0T.imag > 0.0f ? 1.0f : 0.0f);

                    // Phase error
                    phase_error = (((p_0T - p_2T) * c_1T.conj()) - ((c_0T - c_2T) * p_1T.conj())).real;
                    phase_error = dsp::branched_clip(phase_error, 1.0);

                    // Write output
                    obuf[ouc++] = p_0T;
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
#if MM_DO_BRANCH
            memcpy(buffer, &Block<T, T>::input_stream->readBuf[nsamples - pfb.ntaps + 1], (pfb.ntaps - 1) * sizeof(T));
#else
            memmove(&buffer[0], &buffer[nsamples], pfb.ntaps * sizeof(T));
#endif

            oblk.size = ouc;
            outputs[0].fifo->wait_enqueue(oblk);
            iblk.free();

            return false;
        }

        template class MMClockRecoveryBlock<complex_t>;
        template class MMClockRecoveryBlock<float>;
    } // namespace ndsp
} // namespace satdump