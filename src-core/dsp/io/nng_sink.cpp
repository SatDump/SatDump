#include "nng_sink.h"
#include "common/dsp/complex.h"
#include "dsp/block_helpers.h"
#include "logger.h"
#include <complex.h>
#include <nng/protocol/pubsub0/pub.h>
#include <volk/volk_malloc.h>

namespace satdump
{
    namespace ndsp
    {
        template <typename T>
        NNGSinkBlock<T>::NNGSinkBlock() : Block("nng_sink_" + getShortTypeName<T>(), {{"in", getTypeSampleType<T>()}}, {})
        {
        }

        template <typename T>
        NNGSinkBlock<T>::~NNGSinkBlock()
        {
        }

        template <typename T>
        void NNGSinkBlock<T>::start()
        {
            logger->info("Opening TCP socket on " + std::string("tcp://" + address + ":" + std::to_string(port)));

            nng_pub0_open_raw(&sock);
            nng_listener_create(&listener, sock, std::string("tcp://" + address + ":" + std::to_string(port)).c_str());
            nng_listener_start(listener, NNG_FLAG_NONBLOCK);

            Block::start();
        }

        template <typename T>
        void NNGSinkBlock<T>::stop(bool stop_now, bool force)
        {
            Block::stop(stop_now, force);

            nng_listener_close(listener);
        }

        template <typename T>
        bool NNGSinkBlock<T>::work()
        {
            DSPBuffer iblk = inputs[0].fifo->wait_dequeue();

            if (iblk.isTerminator())
            {
                inputs[0].fifo->free(iblk);
                return true;
            }

            nng_send(sock, iblk.getSamples<T>(), iblk.size * sizeof(T), NNG_FLAG_NONBLOCK);

            inputs[0].fifo->free(iblk);

            return false;
        }

        template class NNGSinkBlock<complex_t>;
        template class NNGSinkBlock<float>;
        template class NNGSinkBlock<int16_t>;
        template class NNGSinkBlock<int8_t>;
        template class NNGSinkBlock<uint8_t>;
    } // namespace ndsp
} // namespace satdump