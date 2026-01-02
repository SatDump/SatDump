#include "loris.h"
#include "common/ccsds/ccsds.h"
#include "image/io.h"
#include "utils/time.h"
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <thread>

namespace satdump
{
    namespace ominous
    {
        LORISSimulator::LORISSimulator(WorkerCore core)
            : InstrumentWorkerBase(core), //
              simulator_csr(core.tracker, 110, 2048, "/home/alan/Downloads/CloudyBlueMarble.png")
        {
            start_th();
            start_time = getTime();
        }

        void LORISSimulator::loop()
        {
            double time = getTime();
            auto pos = core.tracker->get_sat_position_at_raw(time);

            ccsds::CCSDSPacket pkt;
            pkt.header.version = 0;
            pkt.header.type = 0;
            pkt.header.secondary_header_flag = false;
            pkt.header.apid = 1;
            pkt.header.sequence_flag = 0b11;
            pkt.header.packet_sequence_count = pkt_index_seq;
            pkt_index_seq++;

            pkt.payload.resize(8 + 160 * 120 * 2 + 2);

            for (int i = 0; i < 8; i++)
                pkt.payload[0 * 8 + i] = ((uint8_t *)&time)[7 - i];

            uint16_t img_r[160 * 120];
            uint16_t img_g[160 * 120];
            uint16_t img_b[160 * 120];
            simulator_csr.get_picture(time, 160, 120, 0, 0, 0, 160, 120, img_r, img_g, img_b);

            for (int x = 0; x < 160; x++)
            {
                for (int y = 0; y < 120; y++)
                {
                    pkt.payload[8 + (y * 160 + x) * 2 + 0] = img_g[y * 160 + x] >> 8;
                    pkt.payload[8 + (y * 160 + x) * 2 + 1] = img_g[y * 160 + x] & 0xFF;
                }
            }

            core.pushPkt(6, pkt);

            time += 60;
            std::this_thread::sleep_until(std::chrono::system_clock::from_time_t(floor(time)) + std::chrono::milliseconds(int(fmod(time, 1) * 1e3)));
        }
    } // namespace ominous
} // namespace satdump