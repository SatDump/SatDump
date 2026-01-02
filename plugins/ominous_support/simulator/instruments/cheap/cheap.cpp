#include "cheap.h"
#include "common/ccsds/ccsds.h"
#include "image/io.h"
#include "utils/time.h"
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <thread>
#include <unistd.h>

namespace satdump
{
    namespace ominous
    {
        CHEAPSimulator::CHEAPSimulator(WorkerCore core)
            : InstrumentWorkerBase(core), //
              simulator_csr(core.tracker, 110, 2048, "/home/alan/Downloads/CloudyBlueMarble.png")
        {
            start_th();
            start_time = getTime();
        }

        void CHEAPSimulator::loop()
        {
            double time = getTime();
            auto pos = core.tracker->get_sat_position_at_raw(time);

            std::vector<uint16_t> img_r(4056 * 3040);
            std::vector<uint16_t> img_g(4056 * 3040);
            std::vector<uint16_t> img_b(4056 * 3040);
            simulator_csr.get_picture(time, 160, 120, 0, 0, 0, 4056, 3040, img_r.data(), img_g.data(), img_b.data());

            std::vector<uint8_t> full_payload;

            full_payload.resize(8 + 4056 * 3040 * 2 + 2);

            for (int i = 0; i < 8; i++)
                full_payload[0 * 8 + i] = ((uint8_t *)&time)[7 - i];

            for (int x = 0; x < 4056; x++)
            {
                for (int y = 0; y < 3040; y++)
                {
                    full_payload[8 + (y * 4056 + x) * 2 + 0] = img_g[y * 4056 + x] >> 8;
                    full_payload[8 + (y * 4056 + x) * 2 + 1] = img_g[y * 4056 + x] & 0xFF;
                }
            }

            for (int pos = 0, i = 0; pos < full_payload.size(); pos += 65536)
            {
                ccsds::CCSDSPacket pkt;
                pkt.header.version = 0;
                pkt.header.type = 0;
                pkt.header.secondary_header_flag = false;
                pkt.header.apid = 1;
                pkt.header.sequence_flag = 0b11;
                pkt.header.packet_sequence_count = i++;

                int remsize = std::min<int>(full_payload.size() - pos, 65536);

                pkt.payload.insert(pkt.payload.end(), full_payload.begin() + pos, full_payload.begin() + pos + remsize);

                // sleep(1);
                core.pushPkt(7, pkt);
            }

            time += 5 * 60;
            std::this_thread::sleep_until(std::chrono::system_clock::from_time_t(floor(time)) + std::chrono::milliseconds(int(fmod(time, 1) * 1e3)));
        }
    } // namespace ominous
} // namespace satdump