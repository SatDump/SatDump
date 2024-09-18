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

    void QuadratureDemodBlock::set_gain(float gain)
    {
        this->gain = 1.0 / gain;
    }

    int QuadratureDemodBlock::process(complex_t *input, int nsamples, float *output)
    {
#if 0
        memcpy(&buffer[1], input, nsamples * sizeof(complex_t));

        volk_32fc_x2_multiply_conjugate_32fc((lv_32fc_t *)&buffer_out[0], (lv_32fc_t *)&buffer[1], (lv_32fc_t *)&buffer[0], nsamples);

        memmove(&buffer[nsamples], &buffer[1], 1 * sizeof(complex_t));

        for (int i = 0; i < nsamples; i++)
            output[i] = gain * fast_atan2f(buffer_out[i].imag, buffer_out[i].real);
#else
        for (int i = 0; i < nsamples; i++)
        {
            float p = atan2f(input[i].imag, input[i].real);
            float phase_diff = p - phase;

            if (phase_diff > M_PI)
                phase_diff -= 2.0f * M_PI;
            else if (phase_diff <= -M_PI)
                phase_diff += 2.0f * M_PI;

            output[i] = phase_diff * gain;
            phase = p;
        }
#endif

        return nsamples;
    }

    void QuadratureDemodBlock::work()
    {
        int nsamples = input_stream->read();
        if (nsamples <= 0)
        {
            input_stream->flush();
            return;
        }

        process(input_stream->readBuf, nsamples, output_stream->writeBuf);

        input_stream->flush();
        output_stream->swap(nsamples);
    }
}