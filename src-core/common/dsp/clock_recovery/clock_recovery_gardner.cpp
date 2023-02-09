#include "clock_recovery_gardner.h"
#include "common/dsp/window/window.h"

#include "logger.h"
#define DO_BRANCH 0

namespace dsp
{
    template <typename T>
    GardnerClockRecoveryBlock<T>::GardnerClockRecoveryBlock(std::shared_ptr<dsp::stream<T>> input, float omega, float omegaGain, float mu, float muGain, float omegaLimit, int nfilt, int ntaps)
        : Block<T, T>(input),
          mu(mu),
          omega(omega),
          omega_gain(omegaGain),
          mu_gain(muGain),
          omega_relative_limit(omegaLimit)
    {
        // Omega setup
        omega_mid = omega;
        omega_limit = omega_relative_limit * omega;

        // Buffer
#if DO_BRANCH
        buffer = create_volk_buffer<T>(pfb.ntaps * 4);
#else
        buffer = create_volk_buffer<T>(STREAM_BUFFER_SIZE);
#endif

        // Init interpolator
        pfb.init(windowed_sinc(nfilt * ntaps, hz_to_rad(0.5 / (double)nfilt, 1.0), window::nuttall, nfilt), nfilt);

        bufs = 20;
    }

    template <typename T>
    GardnerClockRecoveryBlock<T>::~GardnerClockRecoveryBlock()
    {
        volk_free(buffer);
    }

    template <typename T>
    void GardnerClockRecoveryBlock<T>::work()
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
        memcpy(&buffer[pfb.ntaps - 1 + bufs], Block<T, T>::input_stream->readBuf, nsamples * sizeof(T));
#endif
        ouc = 0;

        for (; inc < nsamples && ouc < STREAM_BUFFER_SIZE;)
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
#if DO_BRANCH
                if (inc - offzc < (pfb.ntaps - 1)) // Zero-Crossing sample
                    volk_32f_x2_dot_prod_32f(&zc_sample, &buffer[inc - offzc], pfb.taps[imuz], pfb.ntaps);
                else
                    volk_32f_x2_dot_prod_32f(&zc_sample, &Block<T, T>::input_stream->readBuf[inc - (pfb.ntaps - 1) - offzc], pfb.taps[imuz], pfb.ntaps);
#else
                volk_32f_x2_dot_prod_32f(&zc_sample, &buffer[inc - offzc + bufs], pfb.taps[imuz], pfb.ntaps);
#endif

#if DO_BRANCH
                if (inc < (pfb.ntaps - 1))
                    volk_32f_x2_dot_prod_32f(&sample, &buffer[inc], pfb.taps[imu], pfb.ntaps);
                else
                    volk_32f_x2_dot_prod_32f(&sample, &Block<T, T>::input_stream->readBuf[inc - (pfb.ntaps - 1)], pfb.taps[imu], pfb.ntaps);
#else
                volk_32f_x2_dot_prod_32f(&sample, &buffer[inc + bufs], pfb.taps[imu], pfb.ntaps);
#endif

                // Phase error
                phase_error = zc_sample * (last_sample - sample);

                phase_error = BRANCHLESS_CLIP(phase_error, 1.0);
                last_sample = sample;

                // Write output sample
                Block<T, T>::output_stream->writeBuf[ouc++] = sample;
            }
            if constexpr (std::is_same_v<T, complex_t>)
            {
#if DO_BRANCH
                if (inc - offzc < (pfb.ntaps - 1)) // Zero-Crossing sample
                    volk_32fc_32f_dot_prod_32fc((lv_32fc_t *)&zc_sample, (lv_32fc_t *)&buffer[inc - offzc], pfb.taps[imuz], pfb.ntaps);
                else
                    volk_32fc_32f_dot_prod_32fc((lv_32fc_t *)&zc_sample, (lv_32fc_t *)&Block<T, T>::input_stream->readBuf[inc - (pfb.ntaps - 1) - offzc], pfb.taps[imuz], pfb.ntaps);
#else
                volk_32fc_32f_dot_prod_32fc((lv_32fc_t *)&zc_sample, (lv_32fc_t *)&buffer[inc - offzc + bufs], pfb.taps[imuz], pfb.ntaps);
#endif

#if DO_BRANCH
                if (inc < (pfb.ntaps - 1))
                    volk_32fc_32f_dot_prod_32fc((lv_32fc_t *)&sample, (lv_32fc_t *)&buffer[inc], pfb.taps[imu], pfb.ntaps);
                else
                    volk_32fc_32f_dot_prod_32fc((lv_32fc_t *)&sample, (lv_32fc_t *)&Block<T, T>::input_stream->readBuf[inc - (pfb.ntaps - 1)], pfb.taps[imu], pfb.ntaps);
#else
                volk_32fc_32f_dot_prod_32fc((lv_32fc_t *)&sample, (lv_32fc_t *)&buffer[inc + bufs], pfb.taps[imu], pfb.ntaps);
#endif

                // Phase error
                phase_error = zc_sample.real * (last_sample.real - sample.real) +
                              zc_sample.imag * (last_sample.imag - sample.imag);

                phase_error = BRANCHLESS_CLIP(phase_error, 1.0);
                last_sample = sample;

                // Write output
                Block<T, T>::output_stream->writeBuf[ouc++] = sample;
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
        memmove(&buffer[0], &buffer[nsamples], (pfb.ntaps + bufs) * sizeof(T));
#endif

        Block<T, T>::input_stream->flush();
        Block<T, T>::output_stream->swap(ouc);
    }

    template class GardnerClockRecoveryBlock<complex_t>;
    template class GardnerClockRecoveryBlock<float>;
}