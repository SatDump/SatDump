#include "iasi_reader.h"
#include "common/ccsds/ccsds_time.h"
#include "iasi_brd.h"

namespace metop
{
    namespace iasi
    {
        IASIReader::IASIReader()
        {
            for (int i = 0; i < 8461; i++)
                channels[i].resize(60 * 2);
            lines = 0;
            timestamps.resize(2, -1);
        }

        IASIReader::~IASIReader()
        {
            for (int i = 0; i < 8461; i++)
                channels[i].clear();
        }

        struct unsigned_int16
        {
            operator unsigned short(void) const { return ((a << 8) + b); }

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
            else if (packet.header.apid == 135)
                cnt1 = 0, cnt2 = 1;
            else if (packet.header.apid == 140)
                cnt1 = 1, cnt2 = 0;
            else if (packet.header.apid == 145)
                cnt1 = 0, cnt2 = 0;

            if (counter <= 30 && counter > 0)
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

                        channels[channel][(lines + cnt1) * 60 + 59 - ((counter - 1) * 2 + cnt2)] = raw_result << (16 - sample_length);
                        channel++;
                    }
                }

                if (cnt1)
                    timestamps[lines + 1] = ccsds::parseCCSDSTimeFull(packet, 10957);
                else
                    timestamps[lines + 0] = ccsds::parseCCSDSTimeFull(packet, 10957);
            }

            // Frame counter
            if (counter == 30 && packet.header.apid == 130)
            {
                lines += 2;
                timestamps.resize(lines + 2, -1);
            }

            for (int channel = 0; channel < 8461; channel++)
                channels[channel].resize((lines + 2) * 60);
        }

        image::Image IASIReader::getChannel(int channel) { return image::Image(channels[channel].data(), 16, 30 * 2, lines, 1); }
    } // namespace iasi
} // namespace metop