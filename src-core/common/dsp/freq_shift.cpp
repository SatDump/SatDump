#include "freq_shift.h"
#include <volk/volk.h>
#include "lib/utils.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846 /* pi */
#endif

namespace dsp
{
    FreqShiftBlock::FreqShiftBlock(std::shared_ptr<dsp::stream<std::complex<float>>> input, float samplerate, float shift) : Block(input)
    {
        phase = std::complex<float>(1, 0);
        phase_delta = std::complex<float>(std::cos(2.0f * M_PI * (shift / samplerate)), std::sin(2.0f * M_PI * (shift / samplerate)));
    }

    void FreqShiftBlock::work()
    {
        int nsamples = input_stream->read();
        if (nsamples <= 0)
            return;
        volk_32fc_s32fc_x2_rotator_32fc(output_stream->writeBuf, input_stream->readBuf, phase_delta, &phase, nsamples);
        input_stream->flush();
        output_stream->swap(nsamples);
    }
}