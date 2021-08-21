#include "mwhs2_reader.h"
#include "common/ccsds/ccsds_time.h"

namespace fengyun
{
    namespace mwhs2
    {
        MWHS2Reader::MWHS2Reader()
        {
            lines = 6;
        }

        MWHS2Reader::~MWHS2Reader()
        {
        }

        void MWHS2Reader::work(ccsds::CCSDSPacket &packet)
        {
            if (packet.payload.size() < 1018)
                return;

            time_t currentTime = ccsds::parseCCSDSTime(packet, 0);
            int marker = (packet.payload[35] >> 2) & 0b11;

            if (imageData.count(currentTime) <= 0 && marker == 0)
            {
                imageData.insert(std::pair<time_t, std::array<std::array<unsigned short, 98>, 15>>(currentTime, std::array<std::array<unsigned short, 98>, 15>()));
                lines++;
                lastTime = currentTime;
            }

            if (marker >= 2)
                currentTime = lastTime;

            int pos = 50;
            for (int i = 0; i < 500; i++)
            {
                lineBuf[i] = packet.payload[pos + i * 2 + 0] << 8 | packet.payload[pos + i * 2 + 1];
            }

            for (int i = 0; i < 98; i++)
            {
                if (marker == 0)
                {
                    imageData[currentTime][0][i] = lineBuf[i + 106 * 0];
                    imageData[currentTime][1][i] = lineBuf[i + 106 * 1];
                    imageData[currentTime][2][i] = lineBuf[i + 106 * 2];
                    imageData[currentTime][3][i] = lineBuf[i + 106 * 3];
                }
                else if (marker == 1)
                {
                    imageData[currentTime][4][i] = lineBuf[i + 106 * 0];
                    imageData[currentTime][5][i] = lineBuf[i + 106 * 1];
                    imageData[currentTime][6][i] = lineBuf[i + 106 * 2];
                    imageData[currentTime][7][i] = lineBuf[i + 106 * 3];
                }
                else if (marker == 2)
                {
                    imageData[currentTime][8][i] = lineBuf[i + 106 * 0];
                    imageData[currentTime][9][i] = lineBuf[i + 106 * 1];
                    imageData[currentTime][10][i] = lineBuf[i + 106 * 2];
                    imageData[currentTime][11][i] = lineBuf[i + 106 * 3];
                }
                else if (marker == 3)
                {
                    imageData[currentTime][12][i] = lineBuf[i + 106 * 0];
                    imageData[currentTime][13][i] = lineBuf[i + 106 * 1];
                    imageData[currentTime][14][i] = lineBuf[i + 106 * 2];
                }
            }
        }

        cimg_library::CImg<unsigned short> MWHS2Reader::getChannel(int channel)
        {
            std::vector<std::pair<time_t, std::array<std::array<unsigned short, 98>, 15>>> imageVector(imageData.begin(), imageData.end());

            // Sort by timestamp
            std::sort(imageVector.begin(), imageVector.end(),
                      [](std::pair<time_t, std::array<std::array<unsigned short, 98>, 15>> &el1,
                         std::pair<time_t, std::array<std::array<unsigned short, 98>, 15>> &el2)
                      {
                          return el1.first < el2.first;
                      });

            cimg_library::CImg<unsigned short> img(98, imageVector.size(), 1, 1);

            if (imageVector.size() > 0)
            {
                int line = 0;

                // Reconstitute the image. Works "OK", not perfect...
                for (const std::pair<time_t, std::array<std::array<unsigned short, 98>, 15>> &lineData : imageVector)
                {
                    std::memcpy(&img.data()[line * 98], lineData.second[channel].data(), 2 * 98);
                    line++;
                }

                img.normalize(0, 65535);
                img.equalize(1000);
            }

            return img;
        }
    }
}