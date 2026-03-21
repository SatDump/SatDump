#include "costas_fast.h"
#include "common/dsp/block.h"
#include "dsp/fast_math.h"
#include <cmath>

namespace satdump
{
    namespace ndsp
    {
        CostasFastBlock::CostasFastBlock() : BlockSimple("costas_fast_cc", {{"in", DSP_SAMPLE_TYPE_CF32}}, {{"out", DSP_SAMPLE_TYPE_CF32}}) {}

        CostasFastBlock::~CostasFastBlock() {}

        // Templated to let the compiler do more work. The consts also make it faster for some reason
        template <int ORDER>
        uint32_t CostasFastBlock::process_order(complex_t *ibuf, uint32_t nsamples, complex_t *obuf)
        {
            for (uint32_t i = 0; i < nsamples; i++)
            {
                // Mix input & VCO
                const float tmp_re = ibuf[i].real * pha_re - ibuf[i].imag * pha_im;
                const float tmp_im = ibuf[i].real * pha_im + ibuf[i].imag * pha_re;
                obuf[i].real = tmp_re;
                obuf[i].imag = tmp_im;

                // Calculate error
                float error;
                if constexpr (ORDER == 2) // Order 2, BPSK
                {
                    error = tmp_re * tmp_im;
                }
                else if constexpr (ORDER == 4) // Order 4, QPSK
                {
                    error = (tmp_re > 0.0f ? 1.0f : -1.0f) * tmp_im - (tmp_im > 0.0f ? 1.0f : -1.0f) * tmp_re;
                }
                else // Order 8, 8-PSK
                {
                    constexpr float K = 0.41421356f; // sqrtf(2) - 1;
                    if (fabsf(tmp_re) >= fabsf(tmp_im))
                        error = (tmp_re > 0.0f ? 1.0f : -1.0f) * tmp_im - (tmp_im > 0.0f ? 1.0f : -1.0f) * tmp_re * K;
                    else
                        error = (tmp_re > 0.0f ? 1.0f : -1.0f) * tmp_im * K - (tmp_im > 0.0f ? 1.0f : -1.0f) * tmp_re;
                }

                // Clip error
                error = dsp::branched_clip(error, 1.0f);

                // Compute new freq
                freq += beta * error;

                // Update frequency (phase offset), small angle approximation
                const float df = beta * error;
                const float new_fre_re = fre_re + df * fre_im;
                const float new_fre_im = fre_im - df * fre_re;
                fre_re = new_fre_re;
                fre_im = new_fre_im;

                // Calc new phase offset, small angle approximation
                const float pa = alpha * error;
                const float new_pha_re_off = pha_re + pa * pha_im;
                const float new_pha_im_off = pha_im - pa * pha_re;

                // Advance phase by phase error + freq
                const float new_pha_re = new_pha_re_off * fre_re - new_pha_im_off * fre_im;
                const float new_pha_im = new_pha_re_off * fre_im + new_pha_im_off * fre_re;
                pha_re = new_pha_re;
                pha_im = new_pha_im;

                // Update, prevent small angle approx to add up too much
                if (renorm_ctr++ >= 64)
                {
                    float inv;
                    renorm_ctr = 0;
                    inv = fast_invsqrt(pha_re * pha_re + pha_im * pha_im);
                    pha_re *= inv, pha_im *= inv;
                    inv = fast_invsqrt(fre_re * fre_re + fre_im * fre_im);
                    fre_re *= inv, fre_im *= inv;

                    // Also freq-limit
                    if (freq > freq_limit_max)
                        freq = freq_limit_max, fre_re = freq_limit_min_cpx.real, fre_im = freq_limit_min_cpx.imag;
                    if (freq < freq_limit_min)
                        freq = freq_limit_min, fre_re = freq_limit_max_cpx.real, fre_im = freq_limit_max_cpx.imag;
                }
            }

            return nsamples;
        }

        uint32_t CostasFastBlock::process(complex_t *input, uint32_t nsamples, complex_t *output)
        {
            switch (order)
            {
            case 2:
                return process_order<2>(input, nsamples, output);
            case 4:
                return process_order<4>(input, nsamples, output);
            case 8:
                return process_order<8>(input, nsamples, output);
            default:
                return 0;
            }
        }

    } // namespace ndsp
} // namespace satdump