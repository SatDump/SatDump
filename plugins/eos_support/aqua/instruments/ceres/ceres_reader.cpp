#include "ceres_reader.h"
#include "common/ccsds/ccsds_time.h"

#include "common/utils.h"

namespace aqua
{
    namespace ceres
    {
        CERESReader::CERESReader()
        {
            for (int i = 0; i < 3; i++)
                channels[i].resize(660);

            lines = 0;
        }

        CERESReader::~CERESReader()
        {
            for (int i = 0; i < 3; i++)
                channels[i].clear();
        }

        void CERESReader::work(ccsds::CCSDSPacket &packet)
        {
            // Filter out bad packets
            if (packet.payload.size() != 6988)
                return;

            for (int i = 0; i < 660; i++)
            {
                /*
                Ok so... I started by following the CCSDS packet format provided in the
                Suomi NPP HRD downlink, since that's the only documenation I could find.
                Everything was going well and matched (GIIS packet format + header
                ignored here), until I did the image stuff.
                From there, I expected 3 consecutive 12-bits samples per record, or basically

                Engr. Data (12-bits) - Shortwave sample (12-bits) - Longwave sample (12-bits) - Total channel (12-bits).

                Though on opening in a bit viewer... Here's what I observed!

                Engr. Data? (16-bits) - Shortwave sample (8-bits) - Longwave sample (8-bits) - Total channel (12-bits).

                So from there, I gave up following the specification that didn't match and got it to work with the values
                above, with some saturation on ch1. Maybe the doc doesn't match because it applies to FM-5 / FM-6 and not
                the older FM-1, FM-2, FM-3 and FM-4!? Or maybe a difference between the NPP and EOS formats...
                Anyway. It works.
                */

                uint8_t *ptr = &packet.payload[18 + 10 * i];

                uint16_t longwave = ptr[2] << 4 | (ptr[1] & 0xF);
                uint16_t shortwave = ptr[3] << 4 | ptr[4] >> 4;

                uint8_t v1 = (ptr[4] & 0xF);
                uint8_t v2 = (ptr[5] >> 4);
                uint8_t v3 = (ptr[5] & 0xF);
                uint8_t v4 = (ptr[0] >> 4);

                uint16_t total = v1 << 8 | v2 << 4; //| v3;

                channels[0][lines * 660 + i] = longwave << 4;  // packet.payload[18 + (10 * i) + 3] << 8;
                channels[1][lines * 660 + i] = shortwave << 4; // packet.payload[18 + (10 * i) + 2] << 8;
                channels[2][lines * 660 + i] = total << 4;     //(packet.payload[18 + (10 * i) + 4] << 4 | packet.payload[18 + (10 * i) + 5] >> 4) << 4;

                uint16_t azimuth = ptr[8] << 8 | ptr[9];

                //                printf("Azimuth %f \n", double(azimuth) / 2047.0);
            }

            lines++;

            timestamps.push_back(ccsds::parseCCSDSTimeFull(packet, -4383));
            timestamps.push_back(ccsds::parseCCSDSTimeFull(packet, -4383) + 3.3);

            for (int i = 0; i < 3; i++)
                channels[i].resize((lines + 1) * 660);
        }

        image::Image CERESReader::getImage(int channel)
        {
            image::Image orin(channels[channel].data(), 16, 660, lines, 1);

            image::Image final(16, 330, lines * 2, 1);

            for (int i = 0; i < lines; i++)
            {
                for (int x = 0; x < 330; x++)
                {
                    int offset2 = x + 330 + 5;
                    final.set(0, x, i * 2 + 0, orin.get(0, x + 0, i));
                    if (offset2 >= 0 && offset2 < 660)
                        final.set(0, 329 - x, i * 2 + 1, orin.get(0, offset2, i));
                }
            }

            final.crop(64, 64 + 206);

            return final;
        }
    } // namespace ceres
} // namespace aqua