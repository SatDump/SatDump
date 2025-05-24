#include "hist_disp.h"

namespace satdump
{
    namespace ndsp
    {
        HistogramDisplayBlock::HistogramDisplayBlock() : Block("hist_disp_cc", {{"in", DSP_SAMPLE_TYPE_CF32}}, {}) {}

        HistogramDisplayBlock::~HistogramDisplayBlock() {}

        bool HistogramDisplayBlock::work()
        {
            DSPBuffer iblk;
            inputs[0].fifo->wait_dequeue(iblk);

            if (iblk.isTerminator())
            {
                iblk.free();
                return true;
            }

            complex_t *samples = iblk.getSamples<complex_t>();

            histo.pushComplex(samples, iblk.size);

            iblk.free();
            return false;
        }
    } // namespace ndsp
} // namespace satdump
