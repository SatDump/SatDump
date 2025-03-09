#include "const_disp.h"

namespace satdump
{
    namespace ndsp
    {
        ConstellationDisplayBlock::ConstellationDisplayBlock()
            : Block("const_disp_cc", {{"in", DSP_SAMPLE_TYPE_COMPLEX}}, {})
        {
        }

        ConstellationDisplayBlock::~ConstellationDisplayBlock()
        {
        }

        bool ConstellationDisplayBlock::work()
        {
            DSPBuffer iblk;
            inputs[0].fifo->wait_dequeue(iblk);

            if (iblk.isTerminator())
            {
                iblk.free();
                return true;
            }

            complex_t *samples = iblk.getSamples<complex_t>();

            constel.pushComplex(samples, iblk.size);

            iblk.free();
            return false;
        }
    }
}