#include "ceres_reader.h"

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

                channels[0][lines * 660 + i] = packet.payload[18 + (10 * i) + 3] << 8;
                channels[1][lines * 660 + i] = packet.payload[18 + (10 * i) + 2] << 8;
                channels[2][lines * 660 + i] = (packet.payload[18 + (10 * i) + 4] << 4 | packet.payload[19 + (10 * i) + 5] >> 4) << 4;
            }

            lines++;

            for (int i = 0; i < 3; i++)
                channels[i].resize((lines + 1) * 660);
        }

        image::Image CERESReader::getImage(int channel)
        {
            return image::Image(channels[channel].data(), 16, 660, lines, 1);
        }
    } // namespace ceres
} // namespace aqua