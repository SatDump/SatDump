#pragma once

#include "tcp_proto.h"

#define REMOTE_NETWORK_DISCOVERY_REQPORT 4567
#define REMOTE_NETWORK_DISCOVERY_REPPORT 7890

#define REMOTE_NETWORK_DISCOVERY_REQPKT \
    {                                   \
        (uint8_t)'S',                   \
            (uint8_t)'a',               \
            (uint8_t)'t',               \
            (uint8_t)'D',               \
            (uint8_t)'u',               \
            (uint8_t)'m',               \
            (uint8_t)'p',               \
            (uint8_t)'S',               \
            (uint8_t)'e',               \
            (uint8_t)'r',               \
            (uint8_t)'v',               \
            (uint8_t)'e',               \
            (uint8_t)'r',               \
            (uint8_t)'S',               \
            (uint8_t)'e',               \
            (uint8_t)'a',               \
            (uint8_t)'r',               \
            (uint8_t)'c',               \
            (uint8_t)'h',               \
    }

#define REMOTE_NETWORK_DISCOVERY_REPPKT \
    {                                   \
        (uint8_t)'S',                   \
            (uint8_t)'a',               \
            (uint8_t)'t',               \
            (uint8_t)'D',               \
            (uint8_t)'u',               \
            (uint8_t)'m',               \
            (uint8_t)'p',               \
            (uint8_t)'S',               \
            (uint8_t)'e',               \
            (uint8_t)'r',               \
            (uint8_t)'v',               \
            (uint8_t)'e',               \
            (uint8_t)'r',               \
            (uint8_t)'F',               \
            (uint8_t)'o',               \
            (uint8_t)'u',               \
            (uint8_t)'n',               \
            (uint8_t)'d',               \
    }

namespace dsp
{
    namespace remote
    {
        enum PKTType
        {
            PKT_TYPE_PING,
            PKT_TYPE_SOURCELIST,
            PKT_TYPE_SOURCEOPEN,
            PKT_TYPE_SOURCECLOSE,
            PKT_TYPE_GUI,
            PKT_TYPE_IQ,
            PKT_TYPE_SAMPLERATEFBK,
            PKT_TYPE_SOURCESTART,
            PKT_TYPE_SOURCESTOP,
            PKT_TYPE_SETFREQ,
            PKT_TYPE_SETSETTINGS,
            PKT_TYPE_GETSETTINGS,
            PKT_TYPE_SAMPLERATESET,
            PKT_TYPE_BITDEPTHSET,
        };

        template <typename T>
        inline void sendPacketWithVector(T *tcp, PKTType pkt_type, std::vector<uint8_t> payload = {})
        {
            uint8_t type = (int)pkt_type;
            payload.insert(payload.begin(), &type, &type + 1);
            if (tcp->swrite(payload.data(), payload.size()) <= 0)
                tcp->closeconn();
        }
    }
}
