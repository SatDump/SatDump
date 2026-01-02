#pragma once

#include "../worker.h"
#include "common/net/tcp.h"
#include "common/net/udp.h"

namespace satdump
{
    namespace ominous
    {
        class LRPTUdpSink : public InstrumentWorkerBase
        {
        private:
            bool lrpt_mode = true;

            net::UDPClient *client;
            net::TCPClient *tclient;

        public:
            LRPTUdpSink(WorkerCore core);
            ~LRPTUdpSink();
            void loop();

            void set_lrpt_mode(bool v) { lrpt_mode = v; }
        };
    } // namespace ominous
} // namespace satdump