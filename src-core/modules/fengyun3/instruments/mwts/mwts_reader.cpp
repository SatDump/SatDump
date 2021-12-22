#include "mwts_reader.h"
#include "common/ccsds/ccsds_time.h"
#include <cstring>
#include <algorithm>

namespace fengyun3
{
    namespace mwts
    {
        MWTSReader::MWTSReader()
        {
            lines = 0;
        }

        MWTSReader::~MWTSReader()
        {
        }

        void MWTSReader::work(ccsds::CCSDSPacket &packet)
        {
            if (packet.payload.size() < 1018)
                return;

            double currentTime = ccsds::parseCCSDSTimeFull(packet, 10957) + 12 * 3600;

            int marker = packet.payload[27] >> 4;

            int marker2 = packet.payload[19]; // Some crap to ignore, perhaps instrument info or so?
            if (marker2 != 170)
                return;

            if (imageData.count(currentTime) <= 0 && marker == 1)
            {
                imageData.insert(std::pair<double, std::array<std::array<unsigned short, 60>, 27>>(currentTime, std::array<std::array<unsigned short, 60>, 27>()));
                lines++;
                lastTime = currentTime;
            }

            if (marker >= 2)
                currentTime = lastTime;

            int pos = 60;
            int bitpos = 4;
            for (int i = 0; i < 1000; i++)
            {
                uint8_t bitShiftBuffer[2];
                bitShiftBuffer[0] = packet.payload[pos + i * 2 + 0] << bitpos | packet.payload[pos + i * 2 + 1] >> (8 - bitpos);
                bitShiftBuffer[1] = packet.payload[pos + i * 2 + 1] << bitpos | packet.payload[pos + i * 2 + 2] >> (8 - bitpos);

                lineBuf[i] = bitShiftBuffer[0] << 8 | bitShiftBuffer[1];
            }

            for (int i = 0; i < 15; i++)
            {
                for (int ch = 0; ch < 20; ch++)
                {
                    if (marker == 1)
                        imageData[currentTime][ch][i] = lineBuf[i + 16 * ch];
                    else if (marker == 2)
                        imageData[currentTime][ch][i + 15 * 1] = lineBuf[i + 16 * ch];
                    else if (marker == 3)
                        imageData[currentTime][ch][i + 15 * 2] = lineBuf[i + 16 * ch];
                    else if (marker == 4)
                        imageData[currentTime][ch][i + 15 * 3] = lineBuf[i + 16 * ch];
                }

                for (int ch = 20; ch < 27; ch++)
                {
                    if (marker == 1)
                        imageData[currentTime][ch][i] = lineBuf[i + 16 * ch];
                    else if (marker == 2)
                        imageData[currentTime][ch][i + 15 * 1] = lineBuf[i + 16 * ch];
                    else if (marker == 3)
                        imageData[currentTime][ch][i + 15 * 2] = lineBuf[i + 16 * ch];
                    else if (marker == 4)
                    {
                        if (i > 10)
                            imageData[currentTime][ch][i + 15 * 3] = lineBuf[10 + 16 * ch]; // If I don't this there is a marker getting into the image... Weird
                        else
                            imageData[currentTime][ch][i + 15 * 3] = lineBuf[i + 16 * ch];
                    }
                }
            }
        }

        image::Image<uint16_t> MWTSReader::getChannel(int channel)
        {
            timestamps.clear();
            std::vector<std::pair<double, std::array<std::array<unsigned short, 60>, 27>>> imageVector(imageData.begin(), imageData.end());

            // Sort by timestamp
            std::sort(imageVector.begin(), imageVector.end(),
                      [](std::pair<double, std::array<std::array<unsigned short, 60>, 27>> &el1,
                         std::pair<double, std::array<std::array<unsigned short, 60>, 27>> &el2)
                      {
                          return el1.first < el2.first;
                      });

            image::Image<uint16_t> img(58, imageVector.size(), 1);

            if (imageVector.size() > 0)
            {
                int line = 0;

                // Reconstitute the image. Works "OK", not perfect...
                for (const std::pair<double, std::array<std::array<unsigned short, 60>, 27>> &lineData : imageVector)
                {
                    std::memcpy(&img.data()[line * 58], lineData.second[channel].data(), 2 * 58);
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