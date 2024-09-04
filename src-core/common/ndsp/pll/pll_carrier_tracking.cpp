#include "pll_carrier_tracking.h"
#include "common/dsp/block.h"

#include "common/dsp/utils/fast_trig.h"
#define M_TWOPI (2 * M_PI)

namespace ndsp
{
    PLLCarrierTracking::PLLCarrierTracking()
        : ndsp::Block("pll_carrier_tracking", {{sizeof(complex_t)}}, {{sizeof(complex_t)}})
    {
    }

    void PLLCarrierTracking::start()
    {
        set_params();
        ndsp::buf::init_nafifo_stdbuf<complex_t>(outputs[0], 2, ((ndsp::buf::StdBuf<complex_t> *)inputs[0]->write_buf())->max);
        ndsp::Block::start();
    }

    void PLLCarrierTracking::set_params(nlohmann::json p)
    {
        if (p.contains("max_freq"))
            d_max_freq = p["max_freq"];
        if (p.contains("min_freq"))
            d_min_freq = p["min_freq"];
        if (p.contains("loop_bw"))
            d_loop_bw = p["loop_bw"];

        max_freq = d_max_freq;
        min_freq = d_min_freq;
        damping = 0;
        loop_bw = d_loop_bw;
        phase = 0;
        freq = 0;

        damping = sqrtf(2.0f) / 2.0f;

        float denom = (1.0 + 2.0 * damping * loop_bw + loop_bw * loop_bw);
        alpha = (4 * damping * loop_bw) / denom;
        beta = (4 * loop_bw * loop_bw) / denom;
    }

    void PLLCarrierTracking::work()
    {
        if (!inputs[0]->read())
        {
            auto *rbuf = (ndsp::buf::StdBuf<complex_t> *)inputs[0]->read_buf();
            auto *wbuf = (ndsp::buf::StdBuf<complex_t> *)outputs[0]->write_buf();

            float phase_error = 0;  // Phase error
            complex_t vcoValue = 0; // Frequency offset

            for (int i = 0; i < rbuf->cnt; i++)
            {
                // Generate VCO
                vcoValue = complex_t(dsp::fast_cos(phase), -dsp::fast_sin(phase));

                // Mix with input
                wbuf->dat[i] = rbuf->dat[i] * vcoValue;

                // Compute phase error and clip it
                phase_error = dsp::fast_atan2f(rbuf->dat[i].imag, rbuf->dat[i].real) - phase;
                while (phase_error < -M_PI)
                    phase_error += M_TWOPI;
                while (phase_error > M_PI)
                    phase_error -= M_TWOPI;

                // Get new phase and freq, then wrap it
                freq = freq + beta * phase_error;
                if (freq > max_freq)
                    freq = max_freq;
                else if (freq < min_freq)
                    freq = min_freq;

                // Get new phase
                phase = phase + freq + alpha * phase_error;
                while (phase < -M_PI)
                    phase += M_TWOPI;
                while (phase > M_PI)
                    phase -= M_TWOPI;
            }

            wbuf->cnt = rbuf->cnt;

            outputs[0]->write();
            inputs[0]->flush();
        }
    }
}
