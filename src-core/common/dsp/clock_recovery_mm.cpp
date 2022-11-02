#include "clock_recovery_mm.h"
#include "window.h"

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
        // Get alignement parameters
        int align = volk_get_alignment();

        // Omega setup
        omega_mid = omega;
        omega_limit = omega_relative_limit * omega;

        // Buffer
        in_buffer = 0;
        int size = 2 * STREAM_BUFFER_SIZE;
        buffer = (T *)volk_malloc(size * sizeof(T), align);
        std::fill(buffer, &buffer[size], 0);

        // Init interpolator
        pfb.init(windowed_sinc(nfilt * ntaps, hz_to_rad(0.5 / (double)nfilt, 1.0), window::nuttall, nfilt), nfilt);
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
        memcpy(&buffer[in_buffer], Block<T, T>::input_stream->readBuf, pfb.ntaps * sizeof(T));

        int out_c = 0;                                              // Output index
        int in_c = 0;                                               // Input index
        int input_number = (in_buffer + nsamples) - pfb.ntaps - 16; // Number of samples to use
        float phase_error = 0;                                      // Phase Error
        int output_cnt_max = 2 * omega * nsamples;                  // Max output CNT

        for (; in_c < input_number && out_c < output_cnt_max;)
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
                if (in_c < in_buffer)
                    volk_32f_x2_dot_prod_32f(&sample, &buffer[in_c], pfb.taps[imu], pfb.ntaps);
                else
                    volk_32f_x2_dot_prod_32f(&sample, &Block<T, T>::input_stream->readBuf[in_c - in_buffer], pfb.taps[imu], pfb.ntaps);

                // Phase error
                phase_error = (last_sample < 0 ? -1.0f : 1.0f) * sample - (sample < 0 ? -1.0f : 1.0f) * last_sample;
                phase_error = BRANCHLESS_CLIP(phase_error, 1.0);
                last_sample = sample;

                // Write output sample
                Block<T, T>::output_stream->writeBuf[out_c++] = sample;
            }
            if constexpr (std::is_same_v<T, complex_t>)
            {
                if (in_c < in_buffer)
                    volk_32fc_32f_dot_prod_32fc((lv_32fc_t *)&p_0T, (lv_32fc_t *)&buffer[in_c], pfb.taps[imu], pfb.ntaps);
                else
                    volk_32fc_32f_dot_prod_32fc((lv_32fc_t *)&p_0T, (lv_32fc_t *)&Block<T, T>::input_stream->readBuf[in_c - in_buffer], pfb.taps[imu], pfb.ntaps);

                // Slice it
                c_0T = complex_t(p_0T.real > 0.0f ? 1.0f : 0.0f, p_0T.imag > 0.0f ? 1.0f : 0.0f);

                // Write output
                Block<T, T>::output_stream->writeBuf[out_c++] = p_0T;

                // Phase error
                phase_error = (((p_0T - p_2T) * c_1T.conj()) - ((c_0T - c_2T) * p_1T.conj())).real;
                phase_error = BRANCHLESS_CLIP(phase_error, 1.0);
            }

            // Adjust omega
            omega = omega + omega_gain * phase_error;
            omega = omega_mid + BRANCHLESS_CLIP((omega - omega_mid), omega_limit);

            // Adjust phase
            mu = mu + omega + mu_gain * phase_error;
            in_c += int(floor(mu));
            mu -= floor(mu);
            if (in_c < 0)
                in_c = 0;
        }

        // We need some history for the next run, so copy it over into our buffer
        // If everything's normal this will be around NTAPS - 16 +-1, but the buffer
        // is way larger just by safety
        int to_keep = nsamples - (in_c - in_buffer);
        memcpy(&buffer[0], &Block<T, T>::input_stream->readBuf[in_c - in_buffer], to_keep * sizeof(T));
        in_buffer = to_keep;

        Block<T, T>::input_stream->flush();
        Block<T, T>::output_stream->swap(out_c);
    }

    template class MMClockRecoveryBlock<complex_t>;
    template class MMClockRecoveryBlock<float>;
}