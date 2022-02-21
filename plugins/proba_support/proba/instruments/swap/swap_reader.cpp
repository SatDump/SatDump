#include "swap_reader.h"
#include <fstream>
#include <iostream>
#include <map>
#include <filesystem>
#include "logger.h"
#include "common/image/image.h"
#include "common/image/jpeg_utils.h"
#include "resources.h"
#include "common/ccsds/ccsds_time.h"

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

        void SWAPReader::work(ccsds::CCSDSPacket &packet)
        {
            if (packet.payload.size() < 65530)
                return;

            time_t timestamp = ccsds::parseCCSDSTime(packet, 18630) + 4 * 3600;

            // Start new image
            if (currentOuts.find(timestamp) == currentOuts.end())
            {
                std::tm *timeReadable = gmtime(&timestamp);
                std::string timestamp_str = (timeReadable->tm_mday > 9 ? std::to_string(timeReadable->tm_mday) : "0" + std::to_string(timeReadable->tm_mday)) + "/" +
                                            (timeReadable->tm_mon + 1 > 9 ? std::to_string(timeReadable->tm_mon + 1) : "0" + std::to_string(timeReadable->tm_mon + 1)) + "/" +
                                            std::to_string(timeReadable->tm_year + 1900) + " " +
                                            (timeReadable->tm_hour > 9 ? std::to_string(timeReadable->tm_hour) : "0" + std::to_string(timeReadable->tm_hour)) + ":" +
                                            (timeReadable->tm_min > 9 ? std::to_string(timeReadable->tm_min) : "0" + std::to_string(timeReadable->tm_min)) + ":" +
                                            (timeReadable->tm_sec > 9 ? std::to_string(timeReadable->tm_sec) : "0" + std::to_string(timeReadable->tm_sec));
                logger->info("Found new SWAP image! Saving as SWAP-" + std::to_string(count) + ".jpeg. Timestamp " + timestamp_str);

                std::string utc_filename = "SWAP_" +                                                                                                                    // Instrument name
                                           std::to_string(timeReadable->tm_year + 1900) +                                                                               // Year yyyy
                                           (timeReadable->tm_mon + 1 > 9 ? std::to_string(timeReadable->tm_mon + 1) : "0" + std::to_string(timeReadable->tm_mon + 1)) + // Month MM
                                           (timeReadable->tm_mday > 9 ? std::to_string(timeReadable->tm_mday) : "0" + std::to_string(timeReadable->tm_mday)) + "T" +    // Day dd
                                           (timeReadable->tm_hour > 9 ? std::to_string(timeReadable->tm_hour) : "0" + std::to_string(timeReadable->tm_hour)) +          // Hour HH
                                           (timeReadable->tm_min > 9 ? std::to_string(timeReadable->tm_min) : "0" + std::to_string(timeReadable->tm_min)) +             // Minutes mm
                                           (timeReadable->tm_sec > 9 ? std::to_string(timeReadable->tm_sec) : "0" + std::to_string(timeReadable->tm_sec)) + "Z";        // Seconds ss
                currentOuts.insert(std::pair<time_t, std::pair<int, std::pair<std::string, std::vector<uint8_t>>>>(timestamp, std::pair<int, std::pair<std::string, std::vector<uint8_t>>>(0, std::pair<std::string, std::vector<uint8_t>>(utc_filename, std::vector<uint8_t>()))));
                count++;
            }

            int &currentFrameCount = currentOuts[timestamp].first;
            std::vector<uint8_t> &currentOutVec = currentOuts[timestamp].second.second;

            if (currentFrameCount == 0)
                currentOutVec.insert(currentOutVec.end(), &packet.payload[14 + 78], &packet.payload[14 + 78 + 65530 - 14]);
            else
                currentOutVec.insert(currentOutVec.end(), &packet.payload[14], &packet.payload[14 + 65530 - 14]);

            currentFrameCount++;
        }

        void SWAPReader::save()
        {
            // This is temporary code until a resource system is implemented everywhere.
            image::Image<uint8_t> adc_mask, ffc_mask;
            bool masks_found = false;
            if (resources::resourceExists("proba/swap/adc_mask.png") && resources::resourceExists("proba/swap/ffc_mask.png"))
            {
                adc_mask.load_png(resources::getResourcePath("proba/swap/adc_mask.png"));
                ffc_mask.load_png(resources::getResourcePath("proba/swap/ffc_mask.png"));
                masks_found = true;
            }
            else
            {
                logger->error("Necessary resources were not found, no correction will be applied!");
            }

            for (std::pair<const time_t, std::pair<int, std::pair<std::string, std::vector<uint8_t>>>> &currentPair : currentOuts)
            {
                std::string filename = currentPair.second.second.first;
                std::vector<uint8_t> &currentOutVec = currentPair.second.second.second;

                logger->info("Decompressing " + filename + "...");

                image::Image<uint8_t> img = image::decompress_jpeg(currentOutVec.data(), currentOutVec.size());

                if (img.size() == 0)
                {
                    logger->info("Error! Skipping...");
                    continue;
                }

                if (masks_found)
                {
                    for (int i = 0; i < img.height() * img.width(); i++)
                    {
                        // This was checked against official Proba-2 data
                        img[i] = std::max<float>(0, img[i] - adc_mask[i] * 2.8); // ADC Bias correction
                        img[i] = std::max<float>(0, img[i] - ffc_mask[i] * 2.8); // Flat field correction
                    }
                }

                // Despeckle
                img.simple_despeckle(20);

                logger->info("Good! Saving as png... ");
                WRITE_IMAGE_LOCAL(img, output_folder + "/" + filename + ".png");
            }
        }
    } // namespace swap
} // namespace proba