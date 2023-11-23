#include "costas_loop.h"
#include "common/dsp/utils/fast_math.h"

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
            // vco.real = FastMath::cos(-phase); //  cosf(-phase);
            // vco.imag = FastMath::sin(-phase); // sinf(-phase);
            FastMath::sincos(-phase, vco.imag, vco.real);
            output_stream->writeBuf[i] = input_stream->readBuf[i] * vco;

            // Calculate error
            switch (order)
            {
            case 2: // Order 2, BPSK
                error = output_stream->writeBuf[i].real * output_stream->writeBuf[i].imag;
                break;
            case 4: // Order 4, QPSK
                error = (output_stream->writeBuf[i].real > 0.0f ? 1.0f : -1.0f) * output_stream->writeBuf[i].imag - (output_stream->writeBuf[i].imag > 0.0f ? 1.0f : -1.0f) * output_stream->writeBuf[i].real;
                break;
            case 8: // Order 8, 8-PSK
                const float K = (sqrtf(2.0) - 1);
                if (fabsf(output_stream->writeBuf[i].real) >= fabsf(output_stream->writeBuf[i].imag))
                    error = ((output_stream->writeBuf[i].real > 0.0f ? 1.0f : -1.0f) * output_stream->writeBuf[i].imag - (output_stream->writeBuf[i].imag > 0.0f ? 1.0f : -1.0f) * output_stream->writeBuf[i].real * K);
                else
                    error = ((output_stream->writeBuf[i].real > 0.0f ? 1.0f : -1.0f) * output_stream->writeBuf[i].imag * K - (output_stream->writeBuf[i].imag > 0.0f ? 1.0f : -1.0f) * output_stream->writeBuf[i].real);
                break;
            }

            // Clip error
            // error = branchless_clip(error, 1.0);
            if (error > 1.0)
                error = 1.0;
            else if (error < -1.0)
                error = -1.0;

            // Compute new freq and phase.
            freq += beta * error;
            phase += freq + alpha * error;

            // Clamp freq
            if (freq > 1.0)
                freq = 1.0;
            else if (freq < -1.0)
                freq = -1.0;
        }

        // Wrap phase
        while (phase > (2 * M_PI))
            phase -= 2 * M_PI;
        while (phase < (-2 * M_PI))
            phase += 2 * M_PI;

        input_stream->flush();
        output_stream->swap(nsamples);
    }
}