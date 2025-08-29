#include "hist_disp.h"

namespace satdump
{
    namespace ndsp
    {
        HistogramDisplayBlock::HistogramDisplayBlock() : Block("hist_disp_c", {{"in", DSP_SAMPLE_TYPE_CF32}}, {}) {}

        HistogramDisplayBlock::~HistogramDisplayBlock() {}

        bool HistogramDisplayBlock::work()
        {
            DSPBuffer iblk = inputs[0].fifo->wait_dequeue();

            if (iblk.isTerminator())
            {
                inputs[0].fifo->free(iblk);
                return true;
            }

            complex_t *samples = iblk.getSamples<complex_t>();

            histo.pushComplex(samples, iblk.size);

            inputs[0].fifo->free(iblk);
            return false;
        }
    } // namespace ndsp
} // namespace satdump
