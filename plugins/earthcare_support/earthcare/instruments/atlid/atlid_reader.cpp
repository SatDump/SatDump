#include "atlid_reader.h"
#include "common/ccsds/ccsds_time.h"
#include "common/repack.h"
#include "logger.h"

#include "common/utils.h"

namespace earthcare
{
    namespace atlid
    {
        ATLIDReader::ATLIDReader()
        {
            channel.resize(216);
        }

        ATLIDReader::~ATLIDReader()
        {
        }

        void ATLIDReader::work(ccsds::CCSDSPacket &pkt)
        {
            if (pkt.payload.size() < 429)
                return;

            int id = pkt.payload[23];
            // printf("ID IS %d\n", id);
            if (id != 0)
                return;

            double currentTime = ccsds::parseCCSDSTimeFullRaw(&pkt.payload[2], 3401, 1); 

            for (int i = 0; i < 216; i++)
            {
                uint16_t val = pkt.payload[28 + i * 2 + 0] << 8 | pkt.payload[28 + i * 2 + 1];
                channel[lines * 216 + i] = val;
            }

            lines++;
            timestamps.push_back(currentTime);

            channel.resize((lines + 1) * 216);
        }

        image::Image ATLIDReader::getChannel()
        {
            return image::Image(channel.data(), 16, 216, lines, 1);
        }
    }
}