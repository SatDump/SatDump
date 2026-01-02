#include "navatt.h"
#include "common/ccsds/ccsds.h"
#include "utils/time.h"
#include <chrono>
#include <cstdint>
#include <thread>

namespace satdump
{
    namespace ominous
    {
        void NavAttSimulator::loop()
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

            pkt.payload.resize(8 + 2 * 3 * 8);

            for (int i = 0; i < 8; i++)
                pkt.payload[0 * 8 + i] = ((uint8_t *)&time)[7 - i];

            for (int i = 0; i < 8; i++)
                pkt.payload[1 * 8 + i] = ((uint8_t *)&pos.position[0])[7 - i];
            for (int i = 0; i < 8; i++)
                pkt.payload[2 * 8 + i] = ((uint8_t *)&pos.position[1])[7 - i];
            for (int i = 0; i < 8; i++)
                pkt.payload[3 * 8 + i] = ((uint8_t *)&pos.position[2])[7 - i];

            for (int i = 0; i < 8; i++)
                pkt.payload[4 * 8 + i] = ((uint8_t *)&pos.velocity[0])[7 - i];
            for (int i = 0; i < 8; i++)
                pkt.payload[5 * 8 + i] = ((uint8_t *)&pos.velocity[1])[7 - i];
            for (int i = 0; i < 8; i++)
                pkt.payload[6 * 8 + i] = ((uint8_t *)&pos.velocity[2])[7 - i];

            core.pushPkt(1, pkt);

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    } // namespace ominous
} // namespace satdump