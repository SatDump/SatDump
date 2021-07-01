#include "mwhs_reader.h"
#include "common/ccsds/ccsds_time.h"

namespace fengyun
{
    namespace mwhs
    {
        MWHSReader::MWHSReader()
        {
            lines = 6;
        }

        MWHSReader::~MWHSReader()
        {
        }

        void MWHSReader::work(ccsds::CCSDSPacket &packet)
        {
            if (packet.payload.size() < 1018)
                return;

            time_t currentTime = ccsds::parseCCSDSTime(packet, 0);
            int marker = packet.payload[350] & 2;

            if (imageData.count(currentTime) <= 0 && marker == 0)
            {
                imageData.insert(std::pair<time_t, std::array<std::array<unsigned short, 98>, 6>>(currentTime, std::array<std::array<unsigned short, 98>, 6>()));
                lines++;
                lastTime = currentTime;
            }

            if (marker == 2)
                currentTime = lastTime;

            int pos = 172 + 30 * 6;
            for (int i = 0; i < 500; i++)
            {
                lineBuf[i] = packet.payload[pos + i * 2 + 0] << 8 | packet.payload[pos + i * 2 + 1];
            }

            for (int i = 0; i < 49; i++)
            {
                imageData[currentTime][0][(marker == 0 ? 49 : 0) + i] = lineBuf[i * 6 + 0];
                imageData[currentTime][1][(marker == 0 ? 49 : 0) + i] = lineBuf[i * 6 + 1];
                imageData[currentTime][2][(marker == 0 ? 49 : 0) + i] = lineBuf[i * 6 + 2];
                imageData[currentTime][3][(marker == 0 ? 49 : 0) + i] = lineBuf[i * 6 + 3];
                imageData[currentTime][4][(marker == 0 ? 49 : 0) + i] = lineBuf[i * 6 + 4];
                imageData[currentTime][5][(marker == 0 ? 49 : 0) + i] = lineBuf[i * 6 + 5];
            }
        }

        cimg_library::CImg<unsigned short> MWHSReader::getChannel(int channel)
        {
            std::vector<std::pair<time_t, std::array<std::array<unsigned short, 98>, 6>>> imageVector(imageData.begin(), imageData.end());

            // Sort by timestamp
            std::sort(imageVector.begin(), imageVector.end(),
                      [](std::pair<time_t, std::array<std::array<unsigned short, 98>, 6>> &el1,
                         std::pair<time_t, std::array<std::array<unsigned short, 98>, 6>> &el2)
                      {
                          return el1.first < el2.first;
                      });

            cimg_library::CImg<unsigned short> img(98, imageVector.size(), 1, 1);

            if (imageVector.size() > 0)
            {
                int line = 0;

                // Reconstitute the image. Works "OK", not perfect...
                for (const std::pair<time_t, std::array<std::array<unsigned short, 98>, 6>> &lineData : imageVector)
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