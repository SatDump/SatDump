#include "nng_iq_sink.h"
#include "common/dsp/complex.h"
#include "logger.h"
#include <complex.h>
#include <nng/protocol/pubsub0/pub.h>
#include <volk/volk_malloc.h>

namespace satdump
{
    namespace ndsp
    {
        NNGIQSinkBlock::NNGIQSinkBlock() : Block("nng_iq_sink_c", {{"in", DSP_SAMPLE_TYPE_CF32}}, {}) {}

        NNGIQSinkBlock::~NNGIQSinkBlock() {}

        void NNGIQSinkBlock::start()
        {
            logger->info("Opening TCP socket on " + std::string("tcp://" + address + ":" + std::to_string(port)));

            nng_pub0_open_raw(&sock);
            nng_listener_create(&listener, sock, std::string("tcp://" + address + ":" + std::to_string(port)).c_str());
            nng_listener_start(listener, NNG_FLAG_NONBLOCK);

            Block::start();
        }

        void NNGIQSinkBlock::stop(bool stop_now, bool force)
        {
            Block::stop(stop_now, force);

            nng_listener_close(listener);
        }

        bool NNGIQSinkBlock::work()
        {
            DSPBuffer iblk = inputs[0].fifo->wait_dequeue();

            if (iblk.isTerminator())
            {
                inputs[0].fifo->free(iblk);
                return true;
            }

            nng_send(sock, iblk.getSamples<complex_t>(), iblk.size * sizeof(complex_t), NNG_FLAG_NONBLOCK);

            inputs[0].fifo->free(iblk);

            return false;
        }
    } // namespace ndsp
} // namespace satdump