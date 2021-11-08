#include "clock_recovery_mm.h"
#include "interpolator_taps.h"

namespace dsp
{
    CCMMClockRecoveryBlock::CCMMClockRecoveryBlock(std::shared_ptr<dsp::stream<complex_t>> input, float omega, float omegaGain, float mu, float muGain, float omegaLimit)
        : Block(input),
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
        buffer = (complex_t *)volk_malloc(size * sizeof(complex_t), align);
        std::fill(buffer, &buffer[size], 0);
    }

    CCMMClockRecoveryBlock::~CCMMClockRecoveryBlock()
    {
        volk_free(buffer);
    }

    void CCMMClockRecoveryBlock::work()
    {
        int nsamples = input_stream->read();
        if (nsamples <= 0)
        {
            input_stream->flush();
            return;
        }

        // Copy NTAPS samples in the buffer from input, as that's required for the last samples
        memcpy(&buffer[in_buffer], input_stream->readBuf, NTAPS * sizeof(complex_t));

        int out_c = 0;                                          // Output index
        int in_c = 0;                                           // Input index
        int input_number = (in_buffer + nsamples) - NTAPS - 16; // Number of samples to use
        float phase_error = 0;                                  // Phase Error
        int output_cnt_max = 2 * omega * nsamples;              // Max output CNT

        for (; in_c < input_number && out_c < output_cnt_max;)
        {
            // Propagate delay
            p_2T = p_1T;
            p_1T = p_0T;
            c_2T = c_1T;
            c_1T = c_0T;

            // Compute output
            int imu = (int)rint(mu * NSTEPS);
            if (imu < 0) // If we're out of bounds, clamp
                imu = 0;
            if (imu > NSTEPS)
                imu = NSTEPS;

            if (in_c < in_buffer)
                volk_32fc_32f_dot_prod_32fc((lv_32fc_t *)&p_0T, (lv_32fc_t *)&buffer[in_c], TAPS[imu], NTAPS);
            else
                volk_32fc_32f_dot_prod_32fc((lv_32fc_t *)&p_0T, (lv_32fc_t *)&input_stream->readBuf[in_c - in_buffer], TAPS[imu], NTAPS);

            // Slice it
            c_0T = complex_t(p_0T.real > 0.0f ? 1.0f : 0.0f, p_0T.imag > 0.0f ? 1.0f : 0.0f);

            // Write output
            output_stream->writeBuf[out_c++] = p_0T;

            // Phase error
            phase_error = (((p_0T - p_2T) * c_1T.conj()) - ((c_0T - c_2T) * p_1T.conj())).real;
            phase_error = BRANCHLESS_CLIP(phase_error, 1.0);

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
        memcpy(&buffer[0], &input_stream->readBuf[in_c - in_buffer], to_keep * sizeof(complex_t));
        in_buffer = to_keep;

        input_stream->flush();
        output_stream->swap(out_c);
    }

    FFMMClockRecoveryBlock::FFMMClockRecoveryBlock(std::shared_ptr<dsp::stream<float>> input, float omega, float omegaGain, float mu, float muGain, float omegaLimit)
        : Block(input),
          mu(mu),
          omega(omega),
          omega_gain(omegaGain),
          mu_gain(muGain),
          omega_relative_limit(omegaLimit),
          last_sample(0)
    {
        // Get alignement parameters
        int align = volk_get_alignment();

        // Omega setup
        omega_mid = omega;
        omega_limit = omega_relative_limit * omega;

        // Buffer
        in_buffer = 0;
        int size = 2 * STREAM_BUFFER_SIZE;
        buffer = (float *)volk_malloc(size * sizeof(float), align);
        std::fill(buffer, &buffer[size], 0);
    }

    FFMMClockRecoveryBlock::~FFMMClockRecoveryBlock()
    {
        volk_free(buffer);
    }

    void FFMMClockRecoveryBlock::work()
    {
        int nsamples = input_stream->read();
        if (nsamples <= 0)
        {
            input_stream->flush();
            return;
        }

        // Copy NTAPS samples in the buffer from input, as that's required for the last samples
        memcpy(&buffer[in_buffer], input_stream->readBuf, NTAPS * sizeof(complex_t));

        int out_c = 0;                                          // Output index
        int in_c = 0;                                           // Input index
        int input_number = (in_buffer + nsamples) - NTAPS - 16; // Number of samples to use
        float phase_error = 0;                                  // Phase Error
        float sample = 0;                                       // Output sample
        int output_cnt_max = 2 * omega * nsamples;              // Max output CNT

        for (; in_c < input_number && out_c < output_cnt_max;)
        {
            // Compute output
            int imu = (int)rint(mu * NSTEPS);
            if (imu < 0) // If we're out of bounds, clamp
                imu = 0;
            if (imu > NSTEPS)
                imu = NSTEPS;

            if (in_c < in_buffer)
                volk_32f_x2_dot_prod_32f(&sample, &buffer[in_c], TAPS[imu], NTAPS);
            else
                volk_32f_x2_dot_prod_32f(&sample, &input_stream->readBuf[in_c - in_buffer], TAPS[imu], NTAPS);

            // Phase error
            phase_error = (last_sample < 0 ? -1.0f : 1.0f) * sample - (sample < 0 ? -1.0f : 1.0f) * last_sample;
            phase_error = BRANCHLESS_CLIP(phase_error, 1.0);
            last_sample = sample;

            // Write output sample
            output_stream->writeBuf[out_c++] = sample;

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
        memcpy(&buffer[0], &input_stream->readBuf[in_c - in_buffer], to_keep * sizeof(complex_t));
        in_buffer = to_keep;

        input_stream->flush();
        output_stream->swap(out_c);
    }
}