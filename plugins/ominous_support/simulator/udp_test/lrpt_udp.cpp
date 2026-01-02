#include "lrpt_udp.h"
#include "common/codings/ldpc/labrador/encoder.h"
#include "common/codings/randomization.h"
#include "common/net/udp.h"
#include "logger.h"
#include "simulator/ccsds/ccsds_muxer.h"
#include <chrono>
#include <cstdint>
#include <cstring>
#include <exception>
#include <thread>

namespace satdump
{
    namespace ominous
    {
        LRPTUdpSink::LRPTUdpSink(WorkerCore core) //
            : InstrumentWorkerBase(core)          //
        {
            std::vector<std::pair<int, int>> allowed_vcaps = {
                {1, 1},                                                                                             //
                {5, 101}, {5, 102}, {5, 103}, {5, 104}, {5, 105}, {5, 106}, {5, 107}, {5, 108}, {5, 109}, {5, 110}, //
                {6, 2},                                                                                             //
            };

            std::string addr = "192.168.0.165";
            client = new net::UDPClient((char *)addr.c_str(), 8888);

            tclient = new net::TCPClient((char *)addr.c_str(), 8888);

            core.pkt_bus->register_handler<PushSpacePacketEvent>(
                [this, allowed_vcaps, addr](PushSpacePacketEvent e)
                {
                    bool valid = false;

                    for (auto &v : allowed_vcaps)
                        if (v.first == e.vcid && v.second == e.pkt.header.apid)
                            valid = true;

                    if (valid)
                    {
                        e.pkt.encodeHDR();

                        std::vector<uint8_t> v;
                        v.push_back(e.vcid);
                        v.insert(v.end(), e.pkt.header.raw, e.pkt.header.raw + 6);
                        v.insert(v.end(), e.pkt.payload.begin(), e.pkt.payload.end());

                        logger->info("Send %d", v.size());
#if 0
                        for (int pos = 0, seg = 0; pos < v.size(); pos += 1471)
                        {
                            std::vector<uint8_t> vv;
                            vv.push_back(seg++);
                            logger->trace("Chunk %d : %d / %d", seg, pos, std::min<int>(1471, v.size() - pos));
                            for (int s = 0; s < std::min<int>(1471, v.size() - pos); s++)
                                vv.push_back(v[pos + s]);
                            // vv.insert(vv.end(), v.begin() + pos, v.begin() + std::min<int>(1471, v.size() - pos));
                            client->send(vv.data(), vv.size());
                            std::this_thread::sleep_for(std::chrono::milliseconds(10));
                        }
#else
                        try
                        {
                            //  client->send(v.data(), v.size());
                            //  std::this_thread::sleep_for(std::chrono::milliseconds(10));
                            uint16_t vl = v.size();
                            tclient->sends((uint8_t *)&vl, 2);
                            tclient->sends(v.data(), v.size());
                        }
                        catch (std::exception &e)
                        {
                            if (tclient)
                                delete tclient;
                            tclient = new net::TCPClient((char *)addr.c_str(), 8888);
                        }
#endif
                    }
                });

            //  start_th();
        }

        LRPTUdpSink::~LRPTUdpSink() {}

        void LRPTUdpSink::loop() {}
    } // namespace ominous
} // namespace satdump