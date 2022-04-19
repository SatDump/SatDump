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
                channels[i].resize(90);
            lines = 0;
            testout = std::ofstream("metop_test.bin");
        }

        MHSReader::~MHSReader()
        {
            for (int i = 0; i < 5; i++)
                channels[i].clear();
            testout.close();
        }

        void MHSReader::work(ccsds::CCSDSPacket &packet)
        {
            if (packet.payload.size() < 1302)
                return;

            // Repack samples to 16-bits
            repackBytesTo16bits(&packet.payload[59], 1080, mhs_buffer);

            testout.write((char *)&packet.payload[0], packet.payload.size());

            // Plot into an image
            for (int channel = 0; channel < 5; channel++)
                for (int i = 0; i < 90; i++)
                    channels[channel][lines * 90 + 89 - i] = mhs_buffer[i * 6 + channel + 3];

            timestamps.push_back(ccsds::parseCCSDSTimeFull(packet, 10957));

            // Frame counter
            lines++;

            for (int channel = 0; channel < 5; channel++)
                channels[channel].resize((lines + 1) * 90);
        }

        image::Image<uint16_t> MHSReader::getChannel(int channel)
        {
            return image::Image<uint16_t>(channels[channel].data(), 90, lines, 1);
        }
    } // namespace mhs
} // namespace metop