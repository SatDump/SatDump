#include "udp_sink.h"

namespace dsp
{
    UDPSinkBlock::UDPSinkBlock(std::shared_ptr<dsp::stream<complex_t>> input, char *address, int port)
        : Block(input)
    {
        udp_sender = new net::UDPClient(address, port);
    }

    UDPSinkBlock::~UDPSinkBlock()
    {
        delete udp_sender;
    }

    void UDPSinkBlock::work()
    {
        int nsamples = input_stream->read();
        if (nsamples == 0)
        {
            input_stream->flush();
            return;
        }

        int nsent = 0;
        while (nsamples - nsent > 0)
        {
            int to_send = std::min<int>(nsamples - nsent, 32000 / 8);
            udp_sender->send((uint8_t *)&input_stream->readBuf[nsent], to_send * sizeof(complex_t));
            nsent += to_send;
        }

        input_stream->flush();
    }
}