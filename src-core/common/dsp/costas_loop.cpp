#include "costas_loop.h"

namespace dsp
{
    CostasLoopBlock::CostasLoopBlock(std::shared_ptr<dsp::stream<complex_t>> input, float loop_bw, unsigned int order) : Block(input), order(order), loop_bw(loop_bw)
    {
        float damping = sqrtf(2.0f) / 2.0f;
        float denom = (1.0 + 2.0 * damping * loop_bw + loop_bw * loop_bw);
        alpha = (4 * damping * loop_bw) / denom;
        beta = (4 * loop_bw * loop_bw) / denom;
    }

    void CostasLoopBlock::work()
    {
        int nsamples = input_stream->read();
        if (nsamples <= 0)
        {
            input_stream->flush();
            return;
        }

        for (int i = 0; i < nsamples; i++)
        {
            // Mix input & VCO
            tmp_val = input_stream->readBuf[i] * complex_t(cosf(-phase), sinf(-phase));
            output_stream->writeBuf[i] = tmp_val;

            // Calculate error
            switch (order)
            {
            case 2: // Order 2, BPSK
                error = tmp_val.real * tmp_val.imag;
                break;
            case 4: // Order 4, QPSK
                error = (tmp_val.real > 0.0f ? 1.0f : -1.0f) * tmp_val.imag - (tmp_val.imag > 0.0f ? 1.0f : -1.0f) * tmp_val.real;
                break;
            case 8: // Order 8, 8-PSK
                const float K = (sqrtf(2.0) - 1);
                if (fabsf(tmp_val.real) >= fabsf(tmp_val.imag))
                    error = ((tmp_val.real > 0.0f ? 1.0f : -1.0f) * tmp_val.imag - (tmp_val.imag > 0.0f ? 1.0f : -1.0f) * tmp_val.real * K);
                else
                    error = ((tmp_val.real > 0.0f ? 1.0f : -1.0f) * tmp_val.imag * K - (tmp_val.imag > 0.0f ? 1.0f : -1.0f) * tmp_val.real);
                break;
            }

            // Clip error
            error = branchless_clip(error, 1.0);

            // Compute new freq and phase.
            freq += beta * error;
            phase += freq + alpha * error;

            // Wrap phase
            while (phase > (2 * M_PI))
                phase -= 2 * M_PI;
            while (phase < (-2 * M_PI))
                phase += 2 * M_PI;

            // Clamp freq
            if (freq > 1.0)
                freq = 1.0;
            if (freq < -1.0)
                freq = -1.0;
        }

        input_stream->flush();
        output_stream->swap(nsamples);
    }
}