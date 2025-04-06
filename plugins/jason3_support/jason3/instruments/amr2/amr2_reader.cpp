#include "amr2_reader.h"
#include "common/ccsds/ccsds_time.h"
#include "../timestamp.h"

namespace jason3
{
    namespace amr2
    {
        AMR2Reader::AMR2Reader()
        {
            for (int i = 0; i < 3; i++)
                channels[i].resize(12);
            lines = 0;
        }

        AMR2Reader::~AMR2Reader()
        {
            for (int i = 0; i < 3; i++)
                channels[i].clear();
        }

        void AMR2Reader::work(ccsds::CCSDSPacket &packet)
        {
            if (packet.payload.size() != 186)
                return;

            // We need to know where the satellite was when that packet was created
            time_t currentTime = parseJasonTime(packet); // TODOREWORK!!!!

            // Also write them as a raw images
            for (int i = 0, y = 0; i < 12; i++)
            {
                if (y == 3 || y == 7 || y == 11 || y == 15) // Skip calibration stuff in the middle
                    y++;

                channels[0][lines * 12 + i] = packet.payload[38 + y * 6] << 8 | packet.payload[37 + y * 6];
                channels[1][lines * 12 + i] = packet.payload[38 + y * 6 + 2] << 8 | packet.payload[37 + y * 6 + 2];
                channels[2][lines * 12 + i] = packet.payload[38 + y * 6 + 4] << 8 | packet.payload[37 + y * 6 + 4];

                // TODOREWORK, remove this stupid IF. Just too slow for testing right now otherwise
                if (i == 0)
                {
                    channels_data[0].push_back(channels[0][lines * 12 + i]);
                    channels_data[1].push_back(channels[1][lines * 12 + i]);
                    channels_data[2].push_back(channels[2][lines * 12 + i]);
                    timestamps_data.push_back(currentTime + (double(i) / 12.0));
                }

                y++;
            }

            timestamps.push_back(currentTime);
            lines++;

            for (int channel = 0; channel < 3; channel++)
                channels[channel].resize((lines + 1) * 12);
        }

        image::Image AMR2Reader::getChannel(int channel)
        {
            return image::Image(channels[channel].data(), 16, 12, lines, 1);
        }
    } // namespace modis
} // namespace eos