#include "erm_reader.h"
#include "common/ccsds/ccsds_time.h"
#include <algorithm>
#include <cstring>

namespace fengyun3
{
    namespace erm
    {
        ERMReader::ERMReader()
        {
            lines = 0;
        }

        ERMReader::~ERMReader()
        {
        }

        void ERMReader::work(ccsds::CCSDSPacket &packet)
        {
            if (packet.payload.size() < 1018)
                return;

            double currentTime = ccsds::parseCCSDSTimeFull(packet, 10957) + 12 * 3600;

            if (imageData.count(currentTime) <= 0)
            {
                imageData.insert(std::pair<double, std::array<unsigned short, 151>>(currentTime, std::array<unsigned short, 151>()));
                lines++;
            }

            int pos = 38 + 32 * 2;
            for (int i = 0; i < 151; i++)
            {
                imageData[currentTime][i] = packet.payload[pos + 0] << 8 | packet.payload[pos + 1];
                pos += 2;
            }
        }

        image::Image<uint16_t> ERMReader::getChannel()
        {
            timestamps.clear();
            std::vector<std::pair<double, std::array<unsigned short, 151>>> imageVector(imageData.begin(), imageData.end());

            // Sort by timestamp
            std::sort(imageVector.begin(), imageVector.end(),
                      [](std::pair<double, std::array<unsigned short, 151>> &el1,
                         std::pair<double, std::array<unsigned short, 151>> &el2)
                      {
                          return el1.first < el2.first;
                      });

            image::Image<uint16_t> img(151, imageVector.size(), 1);

            if (imageVector.size() > 0)
            {
                int line = 0;

                // Reconstitute the image. Works "OK", not perfect...
                for (const std::pair<double, std::array<unsigned short, 151>> &lineData : imageVector)
                {
                    std::memcpy(&img.data()[line * 151], lineData.second.data(), 2 * 151);
                    line++;
                    timestamps.push_back(lineData.first);
                }

                img.normalize();
                img.equalize();
            }

            return img;
        }
    }
}