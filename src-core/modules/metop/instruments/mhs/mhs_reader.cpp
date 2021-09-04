#include "mhs_reader.h"
#include "common/ccsds/ccsds_time.h"

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

            int pos = 65 - 6;

            for (int i = 0; i < 90 * 6; i++)
            {
                lineBuffer[i] = packet.payload[pos + 0] << 8 | packet.payload[pos + 1];
                pos += 2;
            }

            // Plot into an image
            for (int channel = 0; channel < 5; channel++)
            {
                for (int i = 0; i < 90; i++)
                {
                    channels[4 - channel][lines * 90 + 89 - i] = lineBuffer[i * 6 + channel + 3];
                }
            }

            timestamps.push_back(ccsds::parseCCSDSTimeFull(packet, 10957));

            // Frame counter
            lines++;
        }

        cimg_library::CImg<unsigned short> MHSReader::getChannel(int channel)
        {
            cimg_library::CImg<unsigned short> img = cimg_library::CImg<unsigned short>(channels[channel], 90, lines);
            img.equalize(1000);
            img.normalize(0, 65535);
            return img;
        }
    } // namespace mhs
} // namespace metop