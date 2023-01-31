#include "pm_to_bpsk.h"
#include "common/dsp/utils/fast_trig.h"
#include <cmath>

#define M_TWOPI (2 * M_PI)

namespace dsp
{
    PMToBPSK::PMToBPSK(std::shared_ptr<dsp::stream<complex_t>> input, float samplerate, float symbolrate) : Block(input)
    {
        // Phase and shifting
        float symbols_per_sample = symbolrate / samplerate;
        phase_shift = M_TWOPI * -symbols_per_sample;
        phase_real = 0;
    }

    void PMToBPSK::work()
    {
        int nsamples = input_stream->read();
        if (nsamples <= 0)
        {
            input_stream->flush();
            return;
        }

        for (int i = 0; i < nsamples; i++)
        {
            shf = complex_t(fast_cos(phase_real), -fast_sin(phase_real));

            output_stream->writeBuf[i] = shf * input_stream->readBuf[i].imag;
            phase_real += phase_shift;

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