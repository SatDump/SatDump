#include "fll_band_edge.h"
#include "common/dsp/utils/fast_trig.h"

#define M_TWOPI (2 * M_PI)

namespace satdump
{
    namespace ndsp
    {
        FLLBandEdgeBlock::FLLBandEdgeBlock() : BlockSimple("fll_band_edge_cc", {{"in", DSP_SAMPLE_TYPE_CF32}}, {{"out", DSP_SAMPLE_TYPE_CF32}}) {}

        FLLBandEdgeBlock::~FLLBandEdgeBlock() {}

        uint32_t FLLBandEdgeBlock::process(complex_t *input, uint32_t nsamples, complex_t *output)
        {
            if (need_reinit)
            {
                init();
                need_reinit = false;
            }

            for (int i = 0; i < nsamples; i++)
            {
                complex_t nco_out = gr_expj(d_phase);
                output[i] = input[i] * nco_out;

                // Perform the dot product of the output with the filters
                complex_t out_upper = filter_low->process(output[i]);
                complex_t out_lower = filter_up->process(output[i]);

                const float error = out_lower.norm() - out_upper.norm();

                d_freq = d_freq + d_beta * error;
                d_phase = d_phase + d_freq + d_alpha * error;

                while (d_phase > (2 * M_PI))
                    d_phase -= 2 * M_PI;
                while (d_phase < (-2 * M_PI))
                    d_phase += 2 * M_PI;

                if (d_freq > max_freq)
                    d_freq = max_freq;
                else if (d_freq < min_freq)
                    d_freq = min_freq;
            }

            return nsamples;
        }
    } // namespace ndsp
} // namespace satdump