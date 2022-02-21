#include "mwri_reader.h"

namespace fengyun3
{
    namespace mwri
    {
        MWRIReader::MWRIReader()
        {
            for (int i = 0; i < 10; i++)
                channels[i].create(10000 * 266);

            lines = 0;
        }

        MWRIReader::~MWRIReader()
        {
            for (int i = 0; i < 10; i++)
                channels[i].destroy();
        }

        void MWRIReader::work(std::vector<uint8_t> &packet)
        {
            if (packet.size() < 7546)
                return;

            for (int ch = 0; ch < 10; ch++)
            {
                for (int i = 0; i < 266; i++)
                {
                    channels[ch][lines * 266 + 265 - i] = packet[200 + ch * 727 + i * 2 + 1] << 8 | packet[200 + ch * 727 + i * 2 + 0];
                }
            }

            // Frame counter
            lines++;

            // Make sure we have enough room
            if (lines * 266 >= (int)channels[0].size())
            {
                for (int i = 0; i < 10; i++)
                    channels[i].resize((lines + 1000) * 266);
            }
        }

        image::Image<uint16_t> MWRIReader::getChannel(int channel)
        {
            image::Image<uint16_t> image(channels[channel].buf, 266, lines, 1);
            image.normalize();
            image.equalize();
            return image;
        }
    } // namespace virr
} // namespace fengyun