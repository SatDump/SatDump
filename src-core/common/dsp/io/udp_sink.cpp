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

        udp_sender->send((uint8_t *)input_stream->readBuf, nsamples * sizeof(complex_t));

        input_stream->flush();
    }
}