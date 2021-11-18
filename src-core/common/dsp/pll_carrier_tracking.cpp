#include "pll_carrier_tracking.h"
#include "fast_trig.h"

namespace dsp
{
    PLLCarrierTrackingBlock::PLLCarrierTrackingBlock(std::shared_ptr<dsp::stream<complex_t>> input, float loop_bw, float max, float min)
        : Block(input),
          d_max_freq(max),
          d_min_freq(min),
          d_damping(0),
          d_loop_bw(loop_bw),
          d_locksig(0),
          d_lock_threshold(0),
          d_squelch_enable(false),
          d_phase(0),
          d_freq(0)
    {
        float denom = (1.0 + 2.0 * d_damping * d_loop_bw + d_loop_bw * d_loop_bw);
        d_alpha = (4 * d_damping * d_loop_bw) / denom;
        d_beta = (4 * d_loop_bw * d_loop_bw) / denom;
    }

    void PLLCarrierTrackingBlock::work()
    {
        int nsamples = input_stream->read();
        if (nsamples <= 0)
        {
            input_stream->flush();
            return;
        }

        float phase_error = 0;  // Phase error
        complex_t vcoValue = 0; // Frequency offset

        for (int i = 0; i < nsamples; i++)
        {
            // Generate VCO
            vcoValue = complex_t(cosf(d_phase), sinf(d_phase));

            // Mix with input
            output_stream->writeBuf[i] = input_stream->readBuf[i] * vcoValue;

            // Compute phase error and clip it
            phase_error = fast_atan2f(input_stream->readBuf[i].imag, input_stream->readBuf[i].real) - d_phase;
            if (phase_error > M_PI)
                phase_error -= (2.0 * M_PI);
            else if (phase_error < -M_PI)
                phase_error += (2.0 * M_PI);

            // Get new phase and freq, then wrap it
            d_freq = d_freq + d_beta * phase_error;
            d_phase = d_phase + d_freq + d_alpha * phase_error;
            while (d_phase > (2 * M_PI))
                d_phase -= 2 * M_PI;
            while (d_phase < (-2 * M_PI))
                d_phase += 2 * M_PI;

            // Optional
            if (d_squelch_enable)
            {
                // Check if we have a lock
                d_locksig = d_locksig * (1.0 - d_alpha) + d_alpha * (input_stream->readBuf[i].real * vcoValue.real + input_stream->readBuf[i].imag * vcoValue.imag);

                // If we don't, set output to 0
                if (d_squelch_enable && !(fabsf(d_locksig) > d_lock_threshold))
                    output_stream->writeBuf[i] = 0;
            }
        }

        input_stream->flush();
        output_stream->swap(nsamples);
    }
}