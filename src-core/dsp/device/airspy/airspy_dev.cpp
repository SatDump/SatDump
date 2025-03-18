#include "airspy_dev.h"
#include "common/dsp/complex.h"

namespace satdump
{
    namespace ndsp
    {
        AirspyDevBlock::AirspyDevBlock()
            : DeviceBlock("airspy_dev_cc", {}, {{"out", DSP_SAMPLE_TYPE_COMPLEX}})
        {
            outputs[0].fifo = std::make_shared<DspBufferFifo>(16); // TODOREWORK
        }

        AirspyDevBlock::~AirspyDevBlock()
        {
        }

        bool AirspyDevBlock::work()
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

            DSPBuffer oblk = DSPBuffer::newBufferSamples<complex_t>(iblk.max_size);
            complex_t *ibuf = iblk.getSamples<complex_t>();

            oblk.size = iblk.size;
            outputs[0].fifo->wait_enqueue(oblk);
            iblk.free();

            return false;
        }
    }
}