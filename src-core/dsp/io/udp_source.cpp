#include "udp_source.h"
#include "dsp/block.h"
#include "dsp/block_helpers.h"
#include <cstdint>
#include <exception>

namespace satdump
{
    namespace ndsp
    {
        template <typename T>
        UDPSourceBlock<T>::UDPSourceBlock()
            : Block("udp_source_" + getShortTypeName<T>(), {}, //
                    {{"out", getTypeSampleType<T>()}})
        {
        }

        template <typename T>
        UDPSourceBlock<T>::~UDPSourceBlock()
        {
        }

        template <typename T>
        bool UDPSourceBlock<T>::work()
        {
            if (work_should_exit)
            {
                outputs[0].fifo->wait_enqueue(outputs[0].fifo->newBufferTerminator());
                return true;
            }

            try
            {
                auto oblk = outputs[0].fifo->newBufferSamples(max_pkt_size, sizeof(T));

                try
                {
                    oblk.size = udp_rx->recv((uint8_t *)oblk.template getSamples<T>(), max_pkt_size);
                }
                catch (std::exception &e)
                {
                    oblk.size = 0;
                }

                if (oblk.size)
                    outputs[0].fifo->wait_enqueue(oblk);
                else
                    outputs[0].fifo->free(oblk);

                return false;
            }
            catch (std::exception &e)
            {
                logger->error("%s", e.what());
                outputs[0].fifo->wait_enqueue(outputs[0].fifo->newBufferTerminator());
                return true;
            }
        }

        template class UDPSourceBlock<complex_t>;
        template class UDPSourceBlock<float>;
        template class UDPSourceBlock<int16_t>;
        template class UDPSourceBlock<int8_t>;
        template class UDPSourceBlock<uint8_t>;
    } // namespace ndsp
} // namespace satdump