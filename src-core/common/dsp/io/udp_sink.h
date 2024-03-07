#pragma once

#include "common/dsp/block.h"
#include "common/net/udp.h"

namespace dsp
{
    class UDPSinkBlock : public Block<complex_t, float>
    {
    private:
        net::UDPClient *udp_sender = nullptr;

        void work();

    public:
        UDPSinkBlock(std::shared_ptr<dsp::stream<complex_t>> input, char *address, int port);
        ~UDPSinkBlock();
    };
}