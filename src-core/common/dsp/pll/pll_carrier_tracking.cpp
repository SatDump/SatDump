#include "pll_carrier_tracking.h"
#include "common/dsp/utils/fast_trig.h"

#define M_TWOPI (2 * M_PI)

namespace dsp
{
    PLLCarrierTrackingBlock::PLLCarrierTrackingBlock(std::shared_ptr<dsp::stream<complex_t>> input, float loop_bw, float max, float min)
        : Block(input),
          d_max_freq(max),
          d_min_freq(min),
          d_damping(0),
          d_loop_bw(loop_bw),
          d_phase(0),
          d_freq(0)
    {
        d_damping = sqrtf(2.0f) / 2.0f;

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
            vcoValue = complex_t(fast_cos(d_phase), -fast_sin(d_phase));

            // Mix with input
            output_stream->writeBuf[i] = input_stream->readBuf[i] * vcoValue;

            // Compute phase error and clip it
            phase_error = fast_atan2f(input_stream->readBuf[i].imag, input_stream->readBuf[i].real) - d_phase;
            while (phase_error < -M_PI)
                phase_error += M_TWOPI;
            while (phase_error > M_PI)
                phase_error -= M_TWOPI;

            // Get new phase and freq, then wrap it
            d_freq = d_freq + d_beta * phase_error;
            if (d_freq > d_max_freq)
                d_freq = d_max_freq;
            else if (d_freq < d_min_freq)
                d_freq = d_min_freq;

            // Get new phase
            d_phase = d_phase + d_freq + d_alpha * phase_error;
            while (d_phase < -M_PI)
                d_phase += M_TWOPI;
            while (d_phase > M_PI)
                d_phase -= M_TWOPI;
        }

        input_stream->flush();
        output_stream->swap(nsamples);
    }
}