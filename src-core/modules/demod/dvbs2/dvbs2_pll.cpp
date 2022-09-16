#include "dvbs2_pll.h"

namespace dvbs2
{
    S2PLLBlock::S2PLLBlock(std::shared_ptr<dsp::stream<complex_t>> input, float loop_bw)
        : Block(input)
    {
        float damping = sqrtf(2.0f) / 2.0f;
        float denom = (1.0 + 2.0 * damping * loop_bw + loop_bw * loop_bw);

        alpha = (4 * damping * loop_bw) / denom;
        beta = (4 * loop_bw * loop_bw) / denom;
    }

    S2PLLBlock::~S2PLLBlock()
    {
    }

    void S2PLLBlock::work()
    {
        int nsamples = input_stream->read();
        if (nsamples <= 0)
        {
            input_stream->flush();
            return;
        }

        for (int i = 0; i < (frame_slot_count + 1) * 90 + pilot_cnt * 36; i++)
        {
            tmp_val = input_stream->readBuf[i] * complex_t(cosf(-phase), sinf(-phase));

            float error = 0;
            if (i >= 90)
            {
                constellation->demod_soft_lut(tmp_val, nullptr, &error);
                output_stream->writeBuf[i] = tmp_val;
            }
            else
            {
                if (i < 26) // Use known symbols for SOF
                    error = (tmp_val * sof.symbols[i].conj()).arg();
                else // And also use known Modcod and config to synchronize onto the PLS
                    error = (tmp_val * pls.symbols[pls_code][i - 26].conj()).arg();

                // We're done, convert to proper 45 degs BPSK
                output_stream->writeBuf[i] = (i & 1) ? complex_t(-tmp_val.real, tmp_val.imag) : complex_t(tmp_val.imag, tmp_val.real);
            }

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