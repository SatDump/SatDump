#pragma once

/**
 * @file block_simple_multi.h
 */

#include "dsp/block.h"
#include <cstdint>

namespace satdump
{
    namespace ndsp
    {
        /**
         * @brief Simplified DSP block implementation, for
         * synchronous blocks that can process one buffer at
         * a time. Takes away the burden of boilerplate code.
         * This very supports multi input/output streams of
         * the SAME type only!
         *
         * @param output_buffer_size_ratio defines the allocated
         * output buffer size relative to the input buffer
         * max_size
         */
        template <typename Ti, typename To, int Ni, int No>
        class BlockSimpleMulti : public Block
        {
        protected:
            float output_buffer_size_ratio[No];

        private:
            Ti *ibufs[Ni];
            uint32_t ibufs_sz[Ni];

            To *obufs[No];
            uint32_t obufs_sz[No];

        private:
            bool work()
            {
                DSPBuffer iblk[Ni];
                uint32_t max_buf_size = 0;
                for (int i = 0; i < Ni; i++)
                {
                    iblk[i] = inputs[i].fifo->wait_dequeue();

                    if (iblk[i].isTerminator())
                    {
                        if (iblk[i].terminatorShouldPropagate())
                            for (int o = 0; o < No; o++)
                                outputs[o].fifo->wait_enqueue(outputs[o].fifo->newBufferTerminator());
                        inputs[i].fifo->free(iblk[i]);
                        return true;
                    }

                    ibufs[i] = iblk[i].template getSamples<Ti>();
                    ibufs_sz[i] = iblk[i].size;

                    if (max_buf_size < iblk[i].max_size)
                        max_buf_size = iblk[i].max_size;
                }

                DSPBuffer oblk[No];
                for (int o = 0; o < No; o++)
                {
                    oblk[o] = outputs[o].fifo->newBufferSamples(ceil(max_buf_size * output_buffer_size_ratio[o]), sizeof(To));
                    obufs[o] = oblk[o].template getSamples<To>();
                }

                process(ibufs, ibufs_sz, obufs, obufs_sz);

                for (int o = 0; o < No; o++)
                {
                    oblk[o].size = obufs_sz[o];
                    outputs[o].fifo->wait_enqueue(oblk[o]);
                }

                for (int i = 0; i < Ni; i++)
                    inputs[i].fifo->free(iblk[i]);

                return false;
            }

        public:
            BlockSimpleMulti(std::string id, std::vector<BlockIO> in = {}, std::vector<BlockIO> out = {}) : Block(id, in, out)
            {
                for (int o = 0; o < No; o++)
                    output_buffer_size_ratio[o] = 1;
            }

            /**
             * @brief Simplified "work" function, called automatically by work().
             * This takes away all boilerplate work usually needed, making simpler
             * block implementations more trivial to work with.
             *
             * The number of output samples MUST be <= nsamples!
             *
             * @param input input sample buffers
             * @param nsamples number of input samples per buffer
             * @param output output sample byffers
             * @param nsamples_out number of output samples per buffer (<= nsamples)
             */
            virtual void process(Ti **input, uint32_t *nsamples, To **output, uint32_t *nsamples_out) = 0;
        };
    } // namespace ndsp
} // namespace satdump