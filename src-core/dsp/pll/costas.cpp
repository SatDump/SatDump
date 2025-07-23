#include "costas.h"
#include "common/dsp/block.h"

namespace satdump
{
    namespace ndsp
    {
        inline float branchless_clip(float x, float clip) { return 0.5 * (std::abs(x + clip) - std::abs(x - clip)); }

        CostasBlock::CostasBlock() : Block("costas_cc", {{"in", DSP_SAMPLE_TYPE_CF32}}, {{"out", DSP_SAMPLE_TYPE_CF32}}) {}

        CostasBlock::~CostasBlock() {}

        bool CostasBlock::work()
        {
            DSPBuffer iblk;
            inputs[0].fifo->wait_dequeue(iblk);

            if (iblk.isTerminator())
            {
                if (iblk.terminatorShouldPropagate())
                    outputs[0].fifo->wait_enqueue(DSPBuffer::newBufferTerminator());
                iblk.free();
                return true;
            }

            auto oblk = DSPBuffer::newBufferSamples<complex_t>(iblk.max_size);
            complex_t *ibuf = iblk.getSamples<complex_t>();
            complex_t *obuf = oblk.getSamples<complex_t>();

            for (uint32_t i = 0; i < iblk.size; i++)
            {
                // Mix input & VCO
                tmp_val = ibuf[i] * complex_t(cosf(-phase), sinf(-phase));
                obuf[i] = tmp_val;

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
                error = dsp::branched_clip(error, 1.0);

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

            oblk.size = iblk.size;
            outputs[0].fifo->wait_enqueue(oblk);
            iblk.free();

            return false;
        }
    } // namespace ndsp
} // namespace satdump