#include "const_disp.h"

namespace satdump
{
    namespace ndsp
    {
        ConstellationDisplayBlock::ConstellationDisplayBlock() : Block("const_disp_c", {{"in", DSP_SAMPLE_TYPE_CF32}}, {}) {}

        ConstellationDisplayBlock::~ConstellationDisplayBlock() {}

        bool ConstellationDisplayBlock::work()
        {
            DSPBuffer iblk = inputs[0].fifo->wait_dequeue();

            if (iblk.isTerminator())
            {
                inputs[0].fifo->free(iblk);
                return true;
            }

            complex_t *samples = iblk.getSamples<complex_t>();

            constel.pushComplex(samples, iblk.size);

            inputs[0].fifo->free(iblk);
            return false;
        }
    } // namespace ndsp
} // namespace satdump
