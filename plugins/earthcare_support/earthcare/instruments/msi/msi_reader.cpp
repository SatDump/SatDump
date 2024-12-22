#include "msi_reader.h"
#include "common/ccsds/ccsds_time.h"
#include "common/repack.h"
#include "logger.h"

#include "common/utils.h"

namespace earthcare
{
    namespace msi
    {
        MSIReader::MSIReader()
        {
            for (int i = 0; i < 7; i++)
                channels[i].resize(360);
        }

        MSIReader::~MSIReader()
        {
        }

        double parseCUC(uint8_t *dat)
        {
            double seconds = dat[1] << 24 |
                             dat[2] << 16 |
                             dat[3] << 8 |
                             dat[4];
            double fseconds = dat[5] << 16 |
                              dat[6] << 8 |
                              dat[7];
            return seconds + fseconds / 16777215.0 + 3657 * 24 * 3600;
        }

        void MSIReader::work(ccsds::CCSDSPacket &pkt)
        {
            if (pkt.payload.size() < 802)
                return;

            int id = pkt.payload[19];
            // printf("ID IS %d\n", id);
            if (id < 1 || id > 7)
                return;

            double currentTime = ccsds::parseCCSDSTimeFullRaw(&pkt.payload[2], 3401, 1); // parseCUC(&pkt.payload[2]);

            for (int i = 0; i < 360; i++)
            {
                uint16_t val = pkt.payload[32 + 24 + i * 2 + 0] << 8 | pkt.payload[32 + 24 + i * 2 + 1];
                channels[id - 1][lines * 360 + i] = val;
            }

            if (id == 7)
            {
                logger->info("%f - %s", currentTime, timestamp_to_string(currentTime).c_str());
                lines++;
                timestamps.push_back(currentTime);
            }

            for (int i = 0; i < 7; i++)
                channels[i].resize((lines + 1) * 360);
        }

        image::Image MSIReader::getChannel(int channel)
        {
            return image::Image(channels[channel].data(), 16, 360, lines, 1);
        }
    }
}