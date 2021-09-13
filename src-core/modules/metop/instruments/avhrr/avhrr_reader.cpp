#include "avhrr_reader.h"
#include "common/ccsds/ccsds_time.h"

namespace metop
{
    namespace avhrr
    {
        AVHRRReader::AVHRRReader()
        {
            for (int i = 0; i < 5; i++)
                channels[i].create(10000 * 2048);
            lines = 0;
        }

        AVHRRReader::~AVHRRReader()
        {
            for (int i = 0; i < 5; i++)
                channels[i].destroy();
        }

        void AVHRRReader::work(ccsds::CCSDSPacket &packet)
        {
            if (packet.payload.size() < 12960)
                return;

            int pos = 14; // AVHRR Data, eg, User Data Field

            // Convert into 10-bits values
            for (int i = 0; i < 2048 * 5 + 100; i += 4)
            {
                avhrrBuffer[i] = (packet.payload[pos + 0] << 2) | (packet.payload[pos + 1] >> 6);
                avhrrBuffer[i + 1] = ((packet.payload[pos + 1] % 64) << 4) | (packet.payload[pos + 2] >> 4);
                avhrrBuffer[i + 2] = ((packet.payload[pos + 2] % 16) << 6) | (packet.payload[pos + 3] >> 2);
                avhrrBuffer[i + 3] = ((packet.payload[pos + 3] % 4) << 8) | packet.payload[pos + 4];
                pos += 5;
            }

            for (int channel = 0; channel < 5; channel++)
            {
                for (int i = 0; i < 2048; i++)
                {
                    uint16_t pixel = avhrrBuffer[55 + channel + i * 5];
                    channels[channel][lines * 2048 + i] = pixel * 60;
                }
            }

            timestamps.push_back(ccsds::parseCCSDSTimeFull(packet, 10957));

            // Frame counter
            lines++;

            // Make sure we have enough room
            if (lines * 2048 >= (int)channels[0].size())
            {
                for (int i = 0; i < 5; i++)
                    channels[i].resize((lines + 1000) * 2048);
            }
        }

        cimg_library::CImg<unsigned short> AVHRRReader::getChannel(int channel)
        {
            return cimg_library::CImg<unsigned short>(channels[channel].buf, 2048, lines);
        }
    } // namespace avhrr
} // namespace metop