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
            for (int band = 0; band < 6; band++)
                for (int channel = 0; channel < 1024; channel++)
                    channels[band][channel].resize(32);
            lines = 0;
        }

        GOMEReader::~GOMEReader()
        {
            for (int band = 0; band < 6; band++)
                for (int channel = 0; channel < 1024; channel++)
                    channels[band][channel].clear();
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

        void GOMEReader::work(ccsds::CCSDSPacket &packet)
        {
            if (packet.payload.size() < 18732)
                return;

            unsigned_int16 *header = (unsigned_int16 *)&packet.payload[14];
            int counter = header[6];

#if 0
            // Detect GOME mode
            if ((counter & 3) == 3)
            {
                int sep_ch1 = header[268];
                int sep_ch2 = header[269];

                band_starts[0] = 0;
                band_starts[1] = sep_ch1 + 1;
                band_starts[2] = 0;
                band_starts[3] = sep_ch2 + 1;
                band_starts[4] = 0;
                band_starts[5] = 0;

                band_ends[0] = sep_ch1;
                band_ends[1] = 1023;
                band_ends[2] = sep_ch2;
                band_ends[3] = 1023;
                band_ends[4] = 1023;
                band_ends[5] = 1023;

                printf("START %d %d %d %d %d %d\n", band_starts[0], band_starts[1], band_starts[2], band_starts[3], band_starts[4], band_starts[5]);
                printf("START %d %d %d %d %d %d\n", band_ends[0], band_ends[1], band_ends[2], band_ends[3], band_ends[4], band_ends[5]);
            }
#endif

            GBand bands[2][4];
            std::memcpy(&bands[0][0], &header[478 + 680], sizeof(bands));

            channel_number = 0;
            for (int band = 0; band < 6; band++)
            {
                int max_channel = (band_ends[band] - band_starts[band] + 1);

                for (int channel = 0; channel < max_channel; channel++)
                {
                    if (band_starts[band] >= max_channel || counter > 15)
                        continue;

                    // There might be a bug on metop?

                    //    if ((header[17] >> (10 - band * 2)) & 3)
                    channels[band][channel][lines * 32 + (31 - (counter * 2 + 0))] = bands[0][band_channels[band]].data[band_starts[band] + channel];
                    //  if ((header[18] >> (5 - band)) & 1)
                    channels[band][channel][lines * 32 + (31 - (counter * 2 + 1))] = bands[1][band_channels[band]].data[band_starts[band] + channel];
                }

                channel_number += max_channel;
            }

            if (counter == 15)
            {
                lines++;
                timestamps.push_back(ccsds::parseCCSDSTimeFull(packet, 10957));
            }

            for (int band = 0; band < 6; band++)
                for (int channel = 0; channel < 1024; channel++)
                    channels[band][channel].resize((lines + 1) * 32);
        }

        image::Image GOMEReader::getChannel(int channel)
        {
            int band = 0;
            int coff = 0;
            int chan = channel;

            while (channel > (coff + (band_ends[band] - band_starts[band] + 1)))
            {
                chan -= band_ends[band] - band_starts[band] + 1;
                coff += band_ends[band] - band_starts[band] + 1;
                band++;
            }

            return image::Image(channels[band][chan].data(), 16, 32, lines, 1);
        }
    } // namespace gome
} // namespace metop