#include "pm_to_bpsk.h"
#include "common/dsp/utils/fast_trig.h"
#include <cmath>

#define M_TWOPI (2 * M_PI)

namespace dsp
{
    PMToBPSK::PMToBPSK(std::shared_ptr<dsp::stream<complex_t>> input, float samplerate, float symbolrate) : Block(input)
    {
        phase_delta = complex_t(cos(hz_to_rad(-symbolrate, samplerate)), sin(hz_to_rad(-symbolrate, samplerate)));
        phase = complex_t(1, 0);
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
            input_stream->readBuf[i] = complex_t(0, input_stream->readBuf[i].imag);

        volk_32fc_s32fc_x2_rotator_32fc((lv_32fc_t *)output_stream->writeBuf, (lv_32fc_t *)input_stream->readBuf, phase_delta, (lv_32fc_t *)&phase, nsamples);

        input_stream->flush();
        output_stream->swap(nsamples);
    }
}