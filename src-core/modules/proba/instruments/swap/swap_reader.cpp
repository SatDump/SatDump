#include "swap_reader.h"
#include <fstream>
#include <iostream>
#include <map>
#include <filesystem>
#include "logger.h"
#include "common/image.h"

#define WRITE_IMAGE_LOCAL(image, path)         \
    image.save_png(std::string(path).c_str()); \
    all_images.push_back(path);

namespace proba
{

    namespace swap
    {
        SWAPReader::SWAPReader(std::string &outputfolder)
        {
            count = 0;
            output_folder = outputfolder;
        }

        void SWAPReader::work(ccsds::ccsds_1_0_proba::CCSDSPacket &packet)
        {
            if (packet.payload.size() < 65530)
                return;

            uint16_t marker = packet.payload[10 - 6] << 8 | packet.payload[11 - 6];

            // Start new image
            if (currentOuts.find(marker) == currentOuts.end())
            {
                logger->info("Found new SWAP image! Saving as SWAP-" + std::to_string(count) + ".jpeg. Marker " + std::to_string(marker));
                currentOuts.insert(std::pair<uint16_t, std::pair<int, std::pair<std::string, std::vector<uint8_t>>>>(marker, std::pair<int, std::pair<std::string, std::vector<uint8_t>>>(0, std::pair<std::string, std::vector<uint8_t>>("SWAP-" + std::to_string(count), std::vector<uint8_t>()))));
                count++;
            }

            int &currentFrameCount = currentOuts[marker].first;
            std::vector<uint8_t> &currentOutVec = currentOuts[marker].second.second;

            if (currentFrameCount == 0)
                currentOutVec.insert(currentOutVec.end(), &packet.payload[14 + 78], &packet.payload[14 + 78 + 65530 - 14]);
            else
                currentOutVec.insert(currentOutVec.end(), &packet.payload[14], &packet.payload[14 + 65530 - 14]);

            currentFrameCount++;
        }

        void SWAPReader::save()
        {
            for (std::pair<const uint16_t, std::pair<int, std::pair<std::string, std::vector<uint8_t>>>> &currentPair : currentOuts)
            {
                std::string filename = currentPair.second.second.first;
                std::vector<uint8_t> &currentOutVec = currentPair.second.second.second;

                logger->info("Decompressing " + filename + "...");

                cimg_library::CImg<unsigned short> img = image::decompress_jpeg(currentOutVec.data(), currentOutVec.size());

                if (img.is_empty())
                {
                    logger->info("Error! Skipping...");
                    continue;
                }

                logger->info("Good! Saving as png... ");
                WRITE_IMAGE_LOCAL(img, output_folder + "/" + filename + ".png");
            }
        }
    } // namespace swap
} // namespace proba