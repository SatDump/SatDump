#include "vco.h"
#include <volk/volk.h>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846 /* pi */
#endif

namespace dsp
{
    VCOBlock::VCOBlock(std::shared_ptr<dsp::stream<float>> input, float k, float amp) : Block(input), d_k(k), d_amp(amp)
    {
    }

    void VCOBlock::work()
    {
        int nsamples = input_stream->read();
        if (nsamples <= 0)
        {
            input_stream->flush();
            return;
        }

        for (int i = 0; i < nsamples; i++)
        {
            output_stream->writeBuf[i].real = cosf(d_phase) * d_amp;
            output_stream->writeBuf[i].imag = sinf(d_phase) * d_amp;
            d_phase += input_stream->readBuf[i] * d_k;

            // Wrap phase
            while (d_phase > (2 * M_PI))
                d_phase -= 2 * M_PI;
            while (d_phase < (-2 * M_PI))
                d_phase += 2 * M_PI;
        }

        input_stream->flush();
        output_stream->swap(nsamples);
    }
}
