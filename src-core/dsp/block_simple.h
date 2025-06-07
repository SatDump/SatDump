#pragma once

/**
 * @file block_simple.h
 */

#include "dsp/block.h"

namespace satdump
{
    namespace ndsp
    {
        /**
         * @brief Simplified DSP block implementation, for
         * synchronous blocks that can process one buffer at
         * a time. Takes away the burden of boilerplate code.
         */
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

            /**
             * @brief Simplified "work" function, called automatically by work().
             * This takes away all boilerplate work usually needed, making simpler
             * block implementations more trivial to work with.
             *
             * The number of output samples MUST be <= nsamples!
             *
             * @param input input sample buffer
             * @param nsamples number of input samples
             * @param output output sample byffer
             * @return number of output samples (<= nsamples)
             */
            virtual uint32_t process(Ti *input, uint32_t nsamples, To *output) = 0;
        };
    } // namespace ndsp
} // namespace satdump