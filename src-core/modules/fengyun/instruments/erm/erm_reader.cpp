#include "erm_reader.h"
#include "common/ccsds/ccsds_time.h"

namespace fengyun
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

            time_t currentTime = ccsds::parseCCSDSTime<ccsds::CCSDSPacket>(packet, 0);

            if (imageData.count(currentTime) <= 0)
            {
                imageData.insert(std::pair<time_t, std::array<unsigned short, 151>>(currentTime, std::array<unsigned short, 151>()));
                lines++;
            }

            int pos = 38 + 32 * 2;
            for (int i = 0; i < 151; i++)
            {
                imageData[currentTime][i] = packet.payload[pos + 0] << 8 | packet.payload[pos + 1];
                pos += 2;
            }
        }

        cimg_library::CImg<unsigned short> ERMReader::getChannel()
        {
            std::vector<std::pair<time_t, std::array<unsigned short, 151>>> imageVector(imageData.begin(), imageData.end());

            // Sort by timestamp
            std::sort(imageVector.begin(), imageVector.end(),
                      [](std::pair<time_t, std::array<unsigned short, 151>> &el1,
                         std::pair<time_t, std::array<unsigned short, 151>> &el2)
                      {
                          return el1.first < el2.first;
                      });

            cimg_library::CImg<unsigned short> img(151, imageVector.size(), 1, 1);

            if (imageVector.size() > 0)
            {
                int line = 0;

                // Reconstitute the image. Works "OK", not perfect...
                for (const std::pair<time_t, std::array<unsigned short, 151>> &lineData : imageVector)
                {
                    std::memcpy(&img.data()[line * 151], lineData.second.data(), 2 * 151);
                    line++;
                }

                img.normalize(0, 65535);
                img.equalize(1000);
            }

            return img;
        }
    }
}