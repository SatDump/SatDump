#include "gome_reader.h"
#include <cstring>
#include "common/ccsds/ccsds_time.h"

#include <cstdio>

namespace metop
{
    namespace gome
    {
        GOMEReader::GOMEReader()
        {
            for (int i = 0; i < 6144; i++)
                channels[i].resize(32);
            lines = 0;
        }

        GOMEReader::~GOMEReader()
        {
            for (int i = 0; i < 6144; i++)
                channels[i].clear();
        }

        struct unsigned_int16
        {
            operator unsigned short(void) const { return ((a << 8) + b); }
            unsigned char a;
            unsigned char b;
        };

        struct GBand
        {
            unsigned_int16 ind;
            unsigned_int16 data[1024];
        };

        // Values extracted from the packet according to the "rough" documenation in the Metopizer
        int band_channels[6] = {0, 0, 1, 1, 2, 3};
        int band_starts[6] = {0, 659, 0, 71, 0, 0};
        int band_ends[6] = {658, 1023, 70, 1023, 1023, 1023};

        void GOMEReader::work(ccsds::CCSDSPacket &packet)
        {
            if (packet.payload.size() < 18732)
                return;

            unsigned_int16 *header = (unsigned_int16 *)&packet.payload[14];
            int counter = header[6];

            // Detect GOME mode
            if ((counter & 3) == 3)
            {
                band_starts[1] = header[268] + 1;
                band_starts[3] = header[269] + 1;
                band_ends[0] = header[268];
                band_ends[2] = header[269];
            }

            GBand bands[2][4];
            std::memcpy(&bands[0][0], &header[478 + 680], sizeof(bands));

            for (int band = 0; band < 6; band++)
            {
                for (int channel = 0; channel < 1024; channel++)
                {
                    if (band_starts[band] >= 1024 || counter > 15)
                        continue;

                    // There might be a bug on metop?

                    //    if ((header[17] >> (10 - band * 2)) & 3)
                    channels[band * 1024 + channel][lines * 32 + (31 - (counter * 2 + 0))] = bands[0][band_channels[band]].data[band_starts[band] + channel];
                    //  if ((header[18] >> (5 - band)) & 1)
                    channels[band * 1024 + channel][lines * 32 + (31 - (counter * 2 + 1))] = bands[1][band_channels[band]].data[band_starts[band] + channel];
                }
            }

            if (counter == 15)
            {
                lines++;
                timestamps.push_back(ccsds::parseCCSDSTimeFull(packet, 10957));
            }

            for (int channel = 0; channel < 6144; channel++)
                channels[channel].resize((lines + 1) * 32);
        }

        image::Image GOMEReader::getChannel(int channel)
        {
            return image::Image(channels[channel].data(), 16, 32, lines, 1);
        }
    } // namespace gome
} // namespace metop