#include "quadrature_demod.h"
#include "common/dsp/block.h"

namespace ndsp
{
    QuadratureDemod::QuadratureDemod()
        : ndsp::Block("quadrature_demod", {{sizeof(complex_t)}}, {{sizeof(float)}})
    {
    }

    void QuadratureDemod::start()
    {
        set_params();
        ndsp::buf::init_nafifo_stdbuf<float>(outputs[0], 2, ((ndsp::buf::StdBuf<float> *)inputs[0]->write_buf())->max);
        ndsp::Block::start();
    }

    void QuadratureDemod::set_params(nlohmann::json p)
    {
        if (p.contains("gain"))
            d_gain = p["gain"];

        gain = d_gain;
    }

    int QuadratureDemod::process(complex_t *input, int nsamples, float *output)
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

    void QuadratureDemod::work()
    {
        if (!inputs[0]->read())
        {
            auto *rbuf = (ndsp::buf::StdBuf<complex_t> *)inputs[0]->read_buf();
            auto *wbuf = (ndsp::buf::StdBuf<float> *)outputs[0]->write_buf();

            wbuf->cnt = process(rbuf->dat, rbuf->cnt, wbuf->dat);

            outputs[0]->write();
            inputs[0]->flush();
        }
    }
}
