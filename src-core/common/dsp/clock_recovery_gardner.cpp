#include "clock_recovery_gardner.h"
#include "interpolator_taps.h"

namespace dsp
{
    GardnerClockRecoveryBlock::GardnerClockRecoveryBlock(std::shared_ptr<dsp::stream<complex_t>> input, float omega, float omegaGain, float mu, float muGain, float omegaLimit)
        : Block(input),
          mu(mu),
          omega(omega),
          omega_gain(omegaGain),
          mu_gain(muGain),
          omega_relative_limit(omegaLimit),
          current_sample(0),
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
        buffer = (complex_t *)volk_malloc(size * sizeof(complex_t), align);
        std::fill(buffer, &buffer[size], 0);
    }

    GardnerClockRecoveryBlock::~GardnerClockRecoveryBlock()
    {
        volk_free(buffer);
    }

    void GardnerClockRecoveryBlock::work()
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
            // Compute output
            int imu = (int)rint(mu * NSTEPS);
            if (imu < 0) // If we're out of bounds, clamp
                imu = 0;
            if (imu > NSTEPS)
                imu = NSTEPS;

            if (in_c < in_buffer)
                volk_32fc_32f_dot_prod_32fc((lv_32fc_t *)&current_sample, (lv_32fc_t *)&buffer[in_c], TAPS[imu], NTAPS);
            else
                volk_32fc_32f_dot_prod_32fc((lv_32fc_t *)&current_sample, (lv_32fc_t *)&input_stream->readBuf[in_c - in_buffer], TAPS[imu], NTAPS);

            // Compute Zero-Crossing output
            imu = (int)rint((mu - omega / 2.0) * NSTEPS);
            if (imu < 0) // If we're out of bounds, clamp
                imu = 0;
            if (imu > NSTEPS)
                imu = NSTEPS;

            if (in_c < in_buffer)
                volk_32fc_32f_dot_prod_32fc((lv_32fc_t *)&zero_crossing_sample, (lv_32fc_t *)&buffer[in_c], TAPS[imu], NTAPS);
            else
                volk_32fc_32f_dot_prod_32fc((lv_32fc_t *)&zero_crossing_sample, (lv_32fc_t *)&input_stream->readBuf[in_c - in_buffer], TAPS[imu], NTAPS);

            // Write output
            output_stream->writeBuf[out_c++] = current_sample;

            // Phase error
            phase_error = zero_crossing_sample.real * (last_sample.real - current_sample.real) +
                          zero_crossing_sample.imag * (last_sample.imag - current_sample.imag);
            phase_error = BRANCHLESS_CLIP(phase_error, 1.0);
            last_sample = current_sample;

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