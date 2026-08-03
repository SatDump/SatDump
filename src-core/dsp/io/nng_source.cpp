#include "nng_source.h"
#include "dsp/block.h"
#include "dsp/block_helpers.h"
#include <cstddef>
#include <cstdint>
#include <exception>
#include <nng/nng.h>
#include <nng/protocol/pubsub0/sub.h>

namespace satdump
{
    namespace ndsp
    {
        template <typename T>
        NNGSourceBlock<T>::NNGSourceBlock()
            : Block("nng_source_" + getShortTypeName<T>(), {}, //
                    {{"out", getTypeSampleType<T>()}})
        {
        }

        template <typename T>
        NNGSourceBlock<T>::~NNGSourceBlock()
        {
        }

        template <typename T>
        void NNGSourceBlock<T>::start()
        {
            logger->info("Opening TCP socket on " + std::string("tcp://" + address + ":" + std::to_string(port)));

            nng_sub0_open_raw(&n_sock);
            nng_dialer_create(&n_dialer, n_sock, std::string("tcp://" + address + ":" + std::to_string(port)).c_str());
            nng_dialer_start(n_dialer, (int)0);

            Block::start();
        }

        template <typename T>
        void NNGSourceBlock<T>::stop(bool stop_now, bool force)
        {
            Block::stop(stop_now, force);

            nng_dialer_close(n_dialer);
            nng_close(n_sock);
        }

        template <typename T>
        bool NNGSourceBlock<T>::work()
        {
            if (work_should_exit)
            {
                outputs[0].fifo->wait_enqueue(outputs[0].fifo->newBufferTerminator());
                return true;
            }

            try
            {
                void *data = NULL;
                size_t lpkt_size;
                nng_recv(n_sock, &data, &lpkt_size, NNG_FLAG_ALLOC | NNG_FLAG_NONBLOCK);

                auto oblk = outputs[0].fifo->newBufferSamples(lpkt_size / sizeof(T), sizeof(T));

                if (data)
                {
                    oblk.size = lpkt_size / sizeof(T);
                    memcpy(oblk.template getSamples<T>(), data, oblk.size * sizeof(T));
                    nng_free(data, lpkt_size);
                }

                if (data)
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

        template class NNGSourceBlock<complex_t>;
        template class NNGSourceBlock<float>;
        template class NNGSourceBlock<int16_t>;
        template class NNGSourceBlock<int8_t>;
        template class NNGSourceBlock<uint8_t>;
    } // namespace ndsp
} // namespace satdump