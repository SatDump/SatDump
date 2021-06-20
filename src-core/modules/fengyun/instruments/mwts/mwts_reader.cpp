#include "mwts_reader.h"
#include "common/ccsds/ccsds_time.h"

namespace fengyun
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

        void MWTSReader::work(ccsds::ccsds_1_0_1024::CCSDSPacket &packet)
        {
            if (packet.payload.size() < 1018)
                return;

            time_t currentTime = ccsds::parseCCSDSTime<ccsds::ccsds_1_0_1024::CCSDSPacket>(packet, 0);

            int marker = packet.payload[19];

            if (marker != 170)
                return;

            if (imageData.count(currentTime) <= 0)
            {
                imageData.insert(std::pair<time_t, std::array<std::array<unsigned short, 15>, 26>>(currentTime, std::array<std::array<unsigned short, 15>, 26>()));
                lines++;
            }

            int pos = 58;
            for (int i = 0; i < 1000; i++)
            {

                lineBuf[i] = packet.payload[pos + i * 2 + 0] << 8 | packet.payload[pos + i * 2 + 1];
            }

            for (int i = 0; i < 15; i++)
            {
                for (int ch = 0; ch < 26; ch++)
                {
                    imageData[currentTime][ch][i] = lineBuf[i + 16 * ch];
                }
            }
        }

        cimg_library::CImg<unsigned short> MWTSReader::getChannel(int channel)
        {
            std::vector<std::pair<time_t, std::array<std::array<unsigned short, 15>, 26>>> imageVector(imageData.begin(), imageData.end());

            // Sort by timestamp
            std::sort(imageVector.begin(), imageVector.end(),
                      [](std::pair<time_t, std::array<std::array<unsigned short, 15>, 26>> &el1,
                         std::pair<time_t, std::array<std::array<unsigned short, 15>, 26>> &el2)
                      {
                          return el1.first < el2.first;
                      });

            cimg_library::CImg<unsigned short> img(15, imageVector.size(), 1, 1);

            if (imageVector.size() > 0)
            {
                int line = 0;

                // Reconstitute the image. Works "OK", not perfect...
                for (const std::pair<time_t, std::array<std::array<unsigned short, 15>, 26>> &lineData : imageVector)
                {
                    std::memcpy(&img.data()[line * 15], lineData.second[channel].data(), 2 * 15);
                    line++;
                }

                img.normalize(0, 65535);
                img.equalize(1000);
            }

            return img;
        }
    }
}