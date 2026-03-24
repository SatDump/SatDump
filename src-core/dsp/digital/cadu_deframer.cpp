#include "cadu_deframer.h"

namespace satdump
{
    namespace ndsp
    {
        CADUDeframerBlock::CADUDeframerBlock() : Block("cadu_deframer_hh", {{"in", DSP_SAMPLE_TYPE_S8}}, {{"out", DSP_SAMPLE_TYPE_S8}}) {}

        void CADUDeframerBlock::init() { def = std::make_shared<deframing::BPSK_CCSDS_Deframer>(cadu_size); }

        CADUDeframerBlock::~CADUDeframerBlock() {}

        bool CADUDeframerBlock::work()
        {
            DSPBuffer iblk = inputs[0].fifo->wait_dequeue();

            if (iblk.isTerminator())
            {
                if (iblk.terminatorShouldPropagate())
                    outputs[0].fifo->wait_enqueue(outputs[0].fifo->newBufferTerminator());
                inputs[0].fifo->free(iblk);
                return true;
            }

            if (needs_reinit)
            {
                needs_reinit = false;
                init();
            }

            DSPBuffer oblk = outputs[0].fifo->newBufferSamples(std::max<int>(iblk.max_size * 2, cadu_size * 10), sizeof(int8_t));
            uint8_t *ibuf = iblk.getSamples<uint8_t>();
            uint8_t *obuf = oblk.getSamples<uint8_t>();

            int n_frm = def->work(ibuf, iblk.size, obuf);

            def_state = def->getState();

            oblk.size = n_frm * (cadu_size / 8);
            if (oblk.size > 0)
                outputs[0].fifo->wait_enqueue(oblk);
            else
                outputs[0].fifo->free(oblk);
            inputs[0].fifo->free(iblk);

            return false;
        }
    } // namespace ndsp
} // namespace satdump