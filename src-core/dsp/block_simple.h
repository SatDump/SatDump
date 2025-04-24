#pragma once

#include "dsp/block.h"

// TODOREWORK document

namespace satdump
{
    namespace ndsp
    {
        template <typename Ti, typename To>
        class BlockSimple : public Block
        {
        private:
            bool work()
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

                DSPBuffer oblk = DSPBuffer::newBufferSamples<To>(iblk.max_size);

                oblk.size = process(iblk.getSamples<Ti>(), iblk.size, oblk.getSamples<To>());

                outputs[0].fifo->wait_enqueue(oblk);
                iblk.free();

                return false;
            }

        public:
            BlockSimple(std::string id, std::vector<BlockIO> in = {}, std::vector<BlockIO> out = {}) : Block(id, in, out) {}

            virtual uint32_t process(Ti *input, uint32_t nsamples, To *output) = 0;
        };
    } // namespace ndsp
} // namespace satdump