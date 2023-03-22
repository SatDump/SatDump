#include "clock_recovery_mm.h"
#include "common/dsp/window/window.h"

#define DO_BRANCH 0

namespace dsp
{
    template <typename T>
    MMClockRecoveryBlock<T>::MMClockRecoveryBlock(std::shared_ptr<dsp::stream<T>> input, float omega, float omegaGain, float mu, float muGain, float omegaLimit, int nfilt, int ntaps)
        : Block<T, T>(input),
          mu(mu),
          omega(omega),
          omega_gain(omegaGain),
          mu_gain(muGain),
          omega_relative_limit(omegaLimit),
          p_2T(0),
          p_1T(0),
          p_0T(0),
          c_2T(0),
          c_1T(0),
          c_0T(0)
    {
        // Omega setup
        omega_mid = omega;
        omega_limit = omega_relative_limit * omega;

        // Init interpolator
        pfb.init(windowed_sinc(nfilt * ntaps, hz_to_rad(0.5 / (double)nfilt, 1.0), window::nuttall, nfilt), nfilt);

        // Buffer
#if DO_BRANCH
        buffer = create_volk_buffer<T>(pfb.ntaps * 4);
#else
        buffer = create_volk_buffer<T>(STREAM_BUFFER_SIZE);
#endif
    }

    template <typename T>
    MMClockRecoveryBlock<T>::~MMClockRecoveryBlock()
    {
        volk_free(buffer);
    }

    template <typename T>
    void MMClockRecoveryBlock<T>::work()
    {
        int nsamples = Block<T, T>::input_stream->read();
        if (nsamples <= 0)
        {
            Block<T, T>::input_stream->flush();
            return;
        }

        // Copy NTAPS samples in the buffer from input, as that's required for the last samples
#if DO_BRANCH
        memcpy(&buffer[pfb.ntaps - 1], Block<T, T>::input_stream->readBuf, (pfb.ntaps - 1) * sizeof(T));
#else
        memcpy(&buffer[pfb.ntaps - 1], Block<T, T>::input_stream->readBuf, nsamples * sizeof(T));
#endif
        ouc = 0;

        for (; inc < nsamples && ouc < STREAM_BUFFER_SIZE;)
        {
            if constexpr (std::is_same_v<T, complex_t>)
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
#if DO_BRANCH
                if (inc < (pfb.ntaps - 1))
                    volk_32f_x2_dot_prod_32f(&sample, &buffer[inc], pfb.taps[imu], pfb.ntaps);
                else
                    volk_32f_x2_dot_prod_32f(&sample, &Block<T, T>::input_stream->readBuf[inc - (pfb.ntaps - 1)], pfb.taps[imu], pfb.ntaps);
#else
                volk_32f_x2_dot_prod_32f(&sample, &buffer[inc], pfb.taps[imu], pfb.ntaps);
#endif

                // Phase error
                phase_error = (last_sample < 0 ? -1.0f : 1.0f) * sample - (sample < 0 ? -1.0f : 1.0f) * last_sample;
                phase_error = BRANCHLESS_CLIP(phase_error, 1.0);
                last_sample = sample;

                // Write output sample
                Block<T, T>::output_stream->writeBuf[ouc++] = sample;
            }
            if constexpr (std::is_same_v<T, complex_t>)
            {
#if DO_BRANCH
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
                phase_error = BRANCHLESS_CLIP(phase_error, 1.0);

                // Write output
                Block<T, T>::output_stream->writeBuf[ouc++] = p_0T;
            }

            // Adjust omega
            omega = omega + omega_gain * phase_error;
            omega = omega_mid + BRANCHLESS_CLIP((omega - omega_mid), omega_limit);

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
#if DO_BRANCH
        memcpy(buffer, &Block<T, T>::input_stream->readBuf[nsamples - pfb.ntaps + 1], (pfb.ntaps - 1) * sizeof(T));
#else
        memmove(&buffer[0], &buffer[nsamples], pfb.ntaps * sizeof(T));
#endif

        Block<T, T>::input_stream->flush();
        Block<T, T>::output_stream->swap(ouc);
    }

    template class MMClockRecoveryBlock<complex_t>;
    template class MMClockRecoveryBlock<float>;
}