#include "gome_reader.h"
#include "utils.h"
#include <cstring>

namespace metop
{
    namespace gome
    {
        GOMEReader::GOMEReader()
        {
            for (int i = 0; i < 6144; i++)
            {
                channels[i] = new unsigned short[10000 * 15];
            }
            lines = 0;
        }

        GOMEReader::~GOMEReader()
        {
            for (int i = 0; i < 6144; i++)
            {
                delete[] channels[i];
            }
        }

        struct unsigned_int16
        {
            operator unsigned short(void) const
            {
                return ((a << 8) + b);
            }

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
                    if (band_starts[band] >= 1024)
                        continue;

                    if ((header[17] >> (10 - band * 2)) & 3)
                    {
                        int val = bands[0][band_channels[band]].data[band_starts[band] + channel];

                        channels[band * 1024 + channel][lines * 15 + counter] = val;
                    }

                    if ((header[18] >> (5 - band)) & 1)
                    {
                        int val = bands[1][band_channels[band]].data[band_starts[band] + channel];

                        channels[band * 1024 + channel][lines * 15 + counter] = val;
                    }
                }
            }

            if (counter == 15)
            {
                lines++;
            }
        }

        image::Image<uint16_t> GOMEReader::getChannel(int channel)
        {
            image::Image<uint16_t> img = image::Image<uint16_t>(channels[channel], 15, lines, 1);
            //img.normalize(0, 65535);
            img.equalize();
            img.mirror(true, false);
            img.resize(30, lines);
            return img;
        }
    } // namespace gome
} // namespace metop