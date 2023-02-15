#include "bpsk_carrier_pll.h"
#include "common/dsp/utils/fast_trig.h"
#include <cmath>

#define M_TWOPI (2 * M_PI)

#ifdef __APPLE__
#define sincosf __sincosf
#endif

#ifdef _WIN32
void sincosf(float x, float *sinx, float *cosx)
{
    *sinx = sinf(x);
    *cosx = cosf(x);
}
#endif

namespace dsp
{
    BPSKCarrierPLLBlock::BPSKCarrierPLLBlock(std::shared_ptr<dsp::stream<complex_t>> input, float alpha, float beta, float max_offset) : Block(input), alpha(alpha), beta(beta), max_offset(max_offset)
    {
    }

    void BPSKCarrierPLLBlock::work()
    {
        int nsamples = input_stream->read();
        if (nsamples <= 0)
        {
            input_stream->flush();
            return;
        }

        for (int i = 0; i < nsamples; i++)
        {
            // Generate and mix out carrier
            float re, im;
            sincosf(phase, &im, &re);
            output_stream->writeBuf[i] = (input_stream->readBuf[i] * complex_t(re, -im)).imag;

            // Compute error and wrap
            float error = fast_atan2f(input_stream->readBuf[i].imag, input_stream->readBuf[i].real) - phase;
            while (error < -M_PI)
                error += M_TWOPI;
            while (error > M_PI)
                error -= M_TWOPI;

            // Clip frequency
            freq = branchless_clip(freq + error * beta, max_offset);

            // Wrap phase
            phase = phase + error * alpha + freq;
            while (phase < -M_PI)
                phase += M_TWOPI;
            while (phase > M_PI)
                phase -= M_TWOPI;
        }

        input_stream->flush();
        output_stream->swap(nsamples);
    }
}