#include "mwrirm_reader.h"

#include "common/repack.h"

namespace fengyun3
{
    namespace mwrirm
    {
        MWRIRMReader::MWRIRMReader()
        {
            for (int i = 0; i < 26; i++)
                channels[i].resize(1000 * 492);

            lines = 0;
        }

        MWRIRMReader::~MWRIRMReader()
        {
            for (int i = 0; i < 26; i++)
                channels[i].clear();
        }

        void MWRIRMReader::work(std::vector<uint8_t> &packet)
        {
            if (packet.size() < 43000)
                return;

            for (int ch = 0; ch < 26; ch++)
            {
                for (int i = 0; i < 492; i++)
                {
                    channels[ch][lines * 492 + 491 - i] = packet[1300 - 146 * 2 - 28 * 2 + ch * 1604 + i * 2 + 0] << 8 | packet[1300 - 146 * 2 - 28 * 2 + ch * 1604 + i * 2 + 1];
                }
            }

            // Parse timestamp
            uint8_t timestamp[8];

            timestamp[0] = packet[18];
            timestamp[1] = packet[19];

            timestamp[2] = packet[20];
            timestamp[3] = packet[21];

            timestamp[4] = packet[22];
            timestamp[5] = packet[23] & 0b11110000;

            uint16_t days = timestamp[0] << 8 | timestamp[1];
            uint64_t milliseconds_of_day = timestamp[2] << 24 | timestamp[3] << 16 | timestamp[4] << 8 | timestamp[5];
            // uint16_t subsecond_cnt = (packet[20] & 0b11) << 8 | packet[21];

            double currentTime = double(10957 + days) * 86400.0 +
                                 double(milliseconds_of_day) / double(1e4) + 12 * 3600;
            ; //+
              //  double(subsecond_cnt) / 512; //+ 12 * 3600;

            timestamps.push_back(currentTime);

            // Frame counter
            lines++;

            // Make sure we have enough room
            for (int i = 0; i < 10; i++)
                channels[i].resize((lines + 1) * 492);
        }

        image::Image<uint16_t> MWRIRMReader::getChannel(int channel)
        {
            return image::Image<uint16_t>(channels[channel].data(), 492, lines, 1);
        }
    } // namespace virr
} // namespace fengyun