#include "pm_pll.h"
#include "fast_trig.h"
#include <cmath>

#define M_TWOPI (2 * M_PI)

namespace dsp
{
    PhaseModulationPLL::PhaseModulationPLL(std::shared_ptr<dsp::stream<complex_t>> input, float loop_bw, float max_offset, float samplerate, float symbolrate) : Block(input), max_offset(max_offset)
    {
        // Loop settings
        float denom = (1.0 + 2.0 * loop_bw + loop_bw * loop_bw);
        alpha = (4 * loop_bw) / denom;
        beta = (4 * loop_bw * loop_bw) / denom;

        // Phase and shifting
        float symbols_per_sample = symbolrate / samplerate;
        phase_shift = M_TWOPI * -symbols_per_sample;
        phase_real = 0;
    }

    void PhaseModulationPLL::work()
    {
        int nsamples = input_stream->read();
        if (nsamples <= 0)
        {
            input_stream->flush();
            return;
        }

        for (int i = 0; i < nsamples; i++)
        {
            // VCOs and mix
            vco = complex_t(fast_cos(phase_carrier), -fast_sin(phase_carrier));
            shf = complex_t(fast_cos(phase_real), -fast_sin(phase_real));
            output_stream->writeBuf[i] = shf * (input_stream->readBuf[i] * vco).imag;
            phase_real += phase_shift;

            // Compute error and wrap
            float error = fast_atan2f(input_stream->readBuf[i].imag, input_stream->readBuf[i].real) - phase_carrier;
            while (error < -M_PI)
                error += M_TWOPI;
            while (error > M_PI)
                error -= M_TWOPI;

            // Clip frequency
            freq = branchless_clip(freq + error * beta, max_offset);

            // Wrap phase
            phase_carrier = phase_carrier + error * alpha + freq; // Correct for offset + increment for PM demod
            while (phase_carrier < -M_PI)
                phase_carrier += M_TWOPI;
            while (phase_carrier > M_PI)
                phase_carrier -= M_TWOPI;

            // Wrap PM phase
            while (phase_real < -M_PI)
                phase_real += M_TWOPI;
            while (phase_real > M_PI)
                phase_real -= M_TWOPI;
        }

        input_stream->flush();
        output_stream->swap(nsamples);
    }
}