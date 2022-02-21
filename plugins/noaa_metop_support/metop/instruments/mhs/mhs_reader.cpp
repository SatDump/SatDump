#include "mhs_reader.h"
#include "common/ccsds/ccsds_time.h"
#include "common/repack.h"

namespace metop
{
    namespace mhs
    {
        MHSReader::MHSReader()
        {
            for (int i = 0; i < 5; i++)
                channels[i] = new unsigned short[10000 * 90];
            lines = 0;
        }

        MHSReader::~MHSReader()
        {
            for (int i = 0; i < 5; i++)
                delete[] channels[i];
        }

        void MHSReader::work(ccsds::CCSDSPacket &packet)
        {
            if (packet.payload.size() < 1302)
                return;

            // Repack samples to 16-bits
            repackBytesTo16bits(&packet.payload[59], 1080, mhs_buffer);

            // Plot into an image
            for (int channel = 0; channel < 5; channel++)
            {
                for (int i = 0; i < 90; i++)
                {
                    channels[channel][lines * 90 + 89 - i] = mhs_buffer[i * 6 + channel + 3];
                }
            }

            timestamps.push_back(ccsds::parseCCSDSTimeFull(packet, 10957));

            // Frame counter
            lines++;
        }

        image::Image<uint16_t> MHSReader::getChannel(int channel)
        {
            image::Image<uint16_t> img = image::Image<uint16_t>(channels[channel], 90, lines, 1);
            return img;
        }
    } // namespace mhs
} // namespace metop