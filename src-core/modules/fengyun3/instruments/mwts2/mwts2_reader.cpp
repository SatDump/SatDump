#include "mwts2_reader.h"
#include "common/ccsds/ccsds_time.h"
#include <cstring>
#include <algorithm>

namespace fengyun3
{
    namespace mwts2
    {
        MWTS2Reader::MWTS2Reader()
        {
            lines = 0;
        }

        MWTS2Reader::~MWTS2Reader()
        {
        }

        void MWTS2Reader::work(ccsds::CCSDSPacket &packet)
        {
            if (packet.payload.size() < 1018)
                return;

            double currentTime = ccsds::parseCCSDSTimeFullRaw(&packet.payload[4], 10957) + 12 * 3600;

            int marker = (packet.payload[0] >> 4) & 0b111;

            if (imageData.count(currentTime) <= 0 && marker == 1)
            {
                imageData.insert(std::pair<double, std::array<std::array<unsigned short, 90>, 18>>(currentTime, std::array<std::array<unsigned short, 90>, 18>()));
                lines++;
                lastTime = currentTime;
            }

            if (marker >= 2)
                currentTime = lastTime;

            int pos = 38;
            int bitpos = 0;
            for (int i = 0; i < 1000; i++)
            {
                uint8_t bitShiftBuffer[2];
                bitShiftBuffer[0] = packet.payload[pos + i * 2 + 0] << bitpos | packet.payload[pos + i * 2 + 1] >> (8 - bitpos);
                bitShiftBuffer[1] = packet.payload[pos + i * 2 + 1] << bitpos | packet.payload[pos + i * 2 + 2] >> (8 - bitpos);

                lineBuf[i] = bitShiftBuffer[0] << 8 | bitShiftBuffer[1];
            }

            for (int i = 0; i < 30; i++)
            {
                for (int ch = 0; ch < 16; ch++)
                {
                    //if (marker == 1)
                    //    imageData[currentTime][ch][i] = lineBuf[i * 16 + ch]; // Packet 1 seems to only contain some calibration or so
                    // Maybe I am discarding some data there but I could not find any matching pixels to the rest of the data
                    if (marker == 2)
                        imageData[currentTime][ch][i + 30 * 0] = lineBuf[i * 16 + ch];
                    else if (marker == 3)
                        imageData[currentTime][ch][i + 30 * 1] = lineBuf[i * 16 + ch];
                    else if (marker == 4)
                        imageData[currentTime][ch][i + 30 * 2] = lineBuf[i * 16 + ch];
                }
            }
        }

        image::Image<uint16_t> MWTS2Reader::getChannel(int channel)
        {
            timestamps.clear();
            std::vector<std::pair<double, std::array<std::array<unsigned short, 90>, 18>>> imageVector(imageData.begin(), imageData.end());

            // Sort by timestamp
            std::sort(imageVector.begin(), imageVector.end(),
                      [](std::pair<double, std::array<std::array<unsigned short, 90>, 18>> &el1,
                         std::pair<double, std::array<std::array<unsigned short, 90>, 18>> &el2)
                      {
                          return el1.first < el2.first;
                      });

            image::Image<uint16_t> img(90, imageVector.size(), 1);

            if (imageVector.size() > 0)
            {
                int line = 0;

                // Reconstitute the image. Works "OK", not perfect...
                for (const std::pair<double, std::array<std::array<unsigned short, 90>, 18>> &lineData : imageVector)
                {
                    std::memcpy(&img.data()[line * 90], lineData.second[channel].data(), 2 * 90);
                    line++;
                    timestamps.push_back(lineData.first);
                }

                img.equalize();
                img.normalize();
            }

            return img;
        }
    }
}