#include "freq_shift.h"
#include <volk/volk.h>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846 /* pi */
#endif

namespace dsp
{
    FreqShiftBlock::FreqShiftBlock(std::shared_ptr<dsp::stream<complex_t>> input, float samplerate, float shift) : Block(input)
    {
        phase = complex_t(1, 0);
        phase_delta = complex_t(cos(hz_to_rad(shift, samplerate)), sin(hz_to_rad(shift, samplerate)));
    }

    void FreqShiftBlock::work()
    {
        int nsamples = input_stream->read();
        if (nsamples <= 0)
        {
            input_stream->flush();
            return;
        }

        volk_32fc_s32fc_x2_rotator_32fc((lv_32fc_t *)output_stream->writeBuf, (lv_32fc_t *)input_stream->readBuf, phase_delta, (lv_32fc_t *)&phase, nsamples);

        input_stream->flush();
        output_stream->swap(nsamples);
    }

    void FreqShiftBlock::set_freq_raw(float freq)
    {
        phase_delta = complex_t(cosf(freq), sinf(freq));
    }
}