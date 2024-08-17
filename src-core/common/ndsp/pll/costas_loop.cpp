#include "costas_loop.h"

namespace ndsp
{
    float branchless_clip(float x, float clip)
    {
        return 0.5 * (std::abs(x + clip) - std::abs(x - clip));
    }

    CostasLoop::CostasLoop()
        : ndsp::Block("costas_loop", {{sizeof(complex_t)}}, {{sizeof(complex_t)}})
    {
    }

    void CostasLoop::start()
    {
        set_params();
        ndsp::buf::init_nafifo_stdbuf<complex_t>(outputs[0], 2, ((ndsp::buf::StdBuf<complex_t> *)inputs[0]->write_buf())->max);
        ndsp::Block::start();
    }

    void CostasLoop::set_params(nlohmann::json p)
    {
        if (p.contains("order"))
            d_order = p["order"];
        if (p.contains("loop_bw"))
            d_loop_bw = p["loop_bw"];
        if (p.contains("freq_limit"))
            d_freq_limit = p["freq_limit"];

        freq_limit_min = -d_freq_limit;
        freq_limit_max = d_freq_limit;

        float damping = sqrtf(2.0f) / 2.0f;
        float denom = (1.0 + 2.0 * damping * d_loop_bw + d_loop_bw * d_loop_bw);
        alpha = (4 * damping * d_loop_bw) / denom;
        beta = (4 * d_loop_bw * d_loop_bw) / denom;
    }

    void CostasLoop::work()
    {
        if (!inputs[0]->read())
        {
            auto *rbuf = (ndsp::buf::StdBuf<complex_t> *)inputs[0]->read_buf();
            auto *wbuf = (ndsp::buf::StdBuf<complex_t> *)outputs[0]->write_buf();

            for (int i = 0; i < rbuf->cnt; i++)
            {
                // Mix input & VCO
                tmp_val = rbuf->dat[i] * complex_t(cosf(-phase), sinf(-phase));
                wbuf->dat[i] = tmp_val;

                // Calculate error
                switch (d_order)
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
                if (freq > freq_limit_max)
                    freq = freq_limit_max;
                if (freq < freq_limit_min)
                    freq = freq_limit_min;
            }

            wbuf->cnt = rbuf->cnt;

            outputs[0]->write();
            inputs[0]->flush();
        }
    }
}
