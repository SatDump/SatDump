#include "net_sink.h"
#include <algorithm>

namespace dsp
{
    NetSinkBlock::NetSinkBlock(std::shared_ptr<dsp::stream<complex_t>> input, std::string smode, char *address, int port)
        : Block(input)
    {
        if (smode == "udp")
            mode = MODE_UDP;
        else if (smode == "nng_pub")
            mode = MODE_NNG_PUB;

        if (mode == MODE_UDP)
        {
            udp_sender = new net::UDPClient(address, port);
        }
        else if (mode == MODE_NNG_PUB)
        {
            logger->info("Opening TCP socket on " + std::string("tcp://" + std::string(address) + ":" + std::to_string(port)));
            nng_pub0_open_raw(&n_sock);
            nng_listener_create(&n_listener, n_sock, std::string("tcp://" + std::string(address) + ":" + std::to_string(port)).c_str());
            nng_listener_start(n_listener, NNG_FLAG_NONBLOCK);
        }
    }

    NetSinkBlock::~NetSinkBlock()
    {
        if (mode == MODE_UDP)
        {
            delete udp_sender;
        }
        else if (mode == MODE_NNG_PUB)
        {
            nng_listener_close(n_listener);
            nng_close(n_sock);
        }
    }

    void NetSinkBlock::work()
    {
        int nsamples = input_stream->read();
        if (nsamples == 0)
        {
            input_stream->flush();
            return;
        }

        if (mode == MODE_UDP)
        {
            int nsent = 0;
            while (nsamples - nsent > 0)
            {
                int to_send = std::min<int>(nsamples - nsent, (1472 * 4) / 8);
                udp_sender->send((uint8_t *)&input_stream->readBuf[nsent], to_send * sizeof(complex_t));
                nsent += to_send;
            }
        }
        else if (mode == MODE_NNG_PUB)
        {
            nng_send(n_sock, &input_stream->readBuf[0], nsamples * sizeof(complex_t), NNG_FLAG_NONBLOCK);
        }

        input_stream->flush();
    }
}