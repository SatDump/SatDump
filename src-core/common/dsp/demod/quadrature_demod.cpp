#include "quadrature_demod.h"
#include <volk/volk.h>
#include "common/dsp/utils/fast_trig.h"

namespace dsp
{
    QuadratureDemodBlock::QuadratureDemodBlock(std::shared_ptr<dsp::stream<complex_t>> input, float gain) : Block(input), gain(1.0 / gain)
    {
        // buffer = create_volk_buffer<complex_t>(STREAM_BUFFER_SIZE);
        // buffer_out = create_volk_buffer<complex_t>(STREAM_BUFFER_SIZE);
    }

    QuadratureDemodBlock::~QuadratureDemodBlock()
    {
        // volk_free(buffer);
        // volk_free(buffer_out);
    }

    void QuadratureDemodBlock::work()
    {
        int nsamples = input_stream->read();
        if (nsamples <= 0)
        {
            input_stream->flush();
            return;
        }

#if 0
        memcpy(&buffer[1], input_stream->readBuf, nsamples * sizeof(complex_t));

        volk_32fc_x2_multiply_conjugate_32fc((lv_32fc_t *)&buffer_out[0], (lv_32fc_t *)&buffer[1], (lv_32fc_t *)&buffer[0], nsamples);

        memmove(&buffer[nsamples], &buffer[1], 1 * sizeof(complex_t));

        for (int i = 0; i < nsamples; i++)
            output_stream->writeBuf[i] = gain * fast_atan2f(buffer_out[i].imag, buffer_out[i].real);
#else
        for (int i = 0; i < nsamples; i++)
        {
            float p = fast_atan2f(input_stream->readBuf[i].imag, input_stream->readBuf[i].real);
            float phase_diff = p - phase;

            if (phase_diff > M_PI)
                phase_diff -= 2.0f * M_PI;
            else if (phase_diff <= -M_PI)
                phase_diff += 2.0f * M_PI;

            output_stream->writeBuf[i] = phase_diff * gain;
            phase = p;
        }
#endif

        input_stream->flush();
        output_stream->swap(nsamples);
    }
}