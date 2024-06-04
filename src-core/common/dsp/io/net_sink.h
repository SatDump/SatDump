#pragma once

#include "common/dsp/block.h"
#include "common/net/udp.h"

#include <nng/nng.h>
#include <nng/protocol/pubsub0/pub.h>

namespace dsp
{
    class NetSinkBlock : public Block<complex_t, float>
    {
    private:
        enum mode_t
        {
            MODE_UDP,
            MODE_NNG_PUB,
        };

        mode_t mode = MODE_UDP;

        net::UDPClient *udp_sender = nullptr;

        nng_socket n_sock;
        nng_listener n_listener;

        void work();

    public:
        NetSinkBlock(std::shared_ptr<dsp::stream<complex_t>> input, std::string mode, char *address, int port);
        ~NetSinkBlock();
    };
}