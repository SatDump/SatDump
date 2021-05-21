#include "costas_loop.h"
#include "utils.h"

namespace dsp
{
    CostasLoop::CostasLoop(float loop_bw, unsigned int order) : ControlLoop(loop_bw, 1.0, -1.0), d_error(0), d_noise(1.0), d_use_snr(false), d_order(order)
    {
        d_damping = sqrtf(2.0f) / 2.0f;

        // Set the bandwidth, which will then call update_gains()
        set_loop_bandwidth(loop_bw);
    }

    int CostasLoop::work(std::complex<float> *in, int length, std::complex<float> *out)
    {
        // Get this out of the for loop if not used:

        for (int i = 0; i < length; i++)
        {
            const std::complex<float> nco_out = gr_expj(-d_phase);

            fast_cc_multiply(out[i], in[i], nco_out);

            // EXPENSIVE LINE with function pointer, switch was about 20% faster in testing.
            // Left in for logic justification/reference. d_error = phase_detector_2(optr[i]);
            switch (d_order)
            {
            case 2:
                d_error = phase_detector_2(out[i]);
                break;
            case 4:
                d_error = phase_detector_4(out[i]);
                break;
            case 8:
                d_error = phase_detector_8(out[i]);
                break;
            }
            d_error = branchless_clip(d_error, 1.0);

            advance_loop(d_error);
            phase_wrap();
            frequency_limit();
        }

        return length;
    }
} // namespace libdsp