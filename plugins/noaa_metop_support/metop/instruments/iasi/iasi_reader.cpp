#include "iasi_reader.h"
#include "iasi_brd.h"

namespace metop
{
    namespace iasi
    {
        IASIReader::IASIReader()
        {
            for (int i = 0; i < 8461; i++)
            {
                channels[i] = new unsigned short[10000 * 30];
            }
            lines = 0;
        }

        IASIReader::~IASIReader()
        {
            for (int i = 0; i < 8461; i++)
            {
                // Already deleted by CImg...
                //delete[] channels[i];
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

        void IASIReader::work(ccsds::CCSDSPacket &packet)
        {
            if (packet.payload.size() < 8954)
                return;

            int counter = packet.payload[16];

            int cnt1 = 0, cnt2 = 0;

            if (packet.header.apid == 130)
                cnt1 = 1, cnt2 = 1;
            if (packet.header.apid == 135)
                cnt1 = 0, cnt2 = 1;
            if (packet.header.apid == 140)
                cnt1 = 1, cnt2 = 0;
            if (packet.header.apid == 145)
                cnt1 = 0, cnt2 = 0;

            if (counter <= 30)
            {
                int bit_pos = 0, channel = 0;
                unsigned_int16 *payload = (unsigned_int16 *)&packet.payload.data()[314];
                int samples_per_segment = IASI_BRD_M02_11::sample_per_segment;
                int no_of_segments = IASI_BRD_M02_11::number_of_segments;

                for (int segment = 0; segment < no_of_segments; segment++)
                {
                    int sample_length = IASI_BRD_M02_11::sample_lengths[segment];

                    for (int sample = 0; sample < samples_per_segment; sample++)
                    {
                        unsigned int raw_result = 0;
                        for (int i = 0; i < sample_length; i++)
                        {
                            unsigned int bit = ((*(payload + bit_pos / 16)) >> ((bit_pos % 16))) & 1;
                            raw_result ^= (bit << i);
                            bit_pos++;
                        }

                        channels[channel][(lines + cnt1) * 60 + (counter - 1) * 2 + cnt2] = ((raw_result));

                        channel++;
                    }
                }
            }

            // Frame counter
            if (counter == 30 && packet.header.apid == 130)
                lines += 2;
        }

        image::Image<uint16_t> IASIReader::getChannel(int channel)
        {
            image::Image<uint16_t> img = image::Image<uint16_t>(channels[channel], 30 * 2, lines, 1);
            //img.normalize();
            img.equalize();
            img.mirror(true, false);
            return img;
        }
    } // namespace iasi
} // namespace metop