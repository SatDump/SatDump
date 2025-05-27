#include "swap_reader.h"
#include <fstream>
#include <iostream>
#include <map>
#include <filesystem>
#include "logger.h"
#include "image/image.h"
#include "image/jpeg_utils.h"
#include "image/io.h"
#include "image/processing.h"
#include "core/resources.h"
#include "common/ccsds/ccsds_time.h"

#define WRITE_IMAGE_LOCAL(img, path)                        \
    {                                                       \
        std::string newPath = path;                         \
        image::append_ext(&newPath);                        \
        image::save_img(img, std::string(newPath).c_str()); \
        all_images.push_back(newPath);                      \
    }

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
            currentOutVec.insert(currentOutVec.end(), &packet.payload[currentFrameCount == 0 ? 14 + 78 : 14], &packet.payload.back());
            currentFrameCount++;
        }

        void SWAPReader::save()
        {
            // This is temporary code until a resource system is implemented everywhere.
            image::Image adc_mask, ffc_mask;
            bool masks_found = false;
            if (resources::resourceExists("proba/swap/adc_mask.png") && resources::resourceExists("proba/swap/ffc_mask.png"))
            {
                image::load_png(adc_mask, resources::getResourcePath("proba/swap/adc_mask.png"));
                image::load_png(ffc_mask, resources::getResourcePath("proba/swap/ffc_mask.png"));
                masks_found = true;
            }
            else
            {
                logger->error("Necessary resources were not found, no correction will be applied!");
            }

            for (std::pair<const time_t, std::pair<int, std::pair<std::string, std::vector<uint8_t>>>> &currentPair : currentOuts)
            {
                std::string extension = "";
                std::string filename = currentPair.second.second.first;
                std::vector<uint8_t> &currentOutVec = currentPair.second.second.second;

                logger->info("Decompressing " + filename + "...");

                image::Image img = image::decompress_jpeg(currentOutVec.data(), currentOutVec.size());

                if (img.size() == 0)
                {
                    logger->info("Error! Skipping...");
                    continue;
                }

                if (masks_found)
                {
                    for (size_t i = 0; i < img.height() * img.width(); i++)
                    {
                        // This was checked against official Proba-2 data
                        img.setf(i, std::max<float>(0, img.getf(i) - adc_mask.getf(i) * 2.8)); // ADC Bias correction
                        img.setf(i, std::max<float>(0, img.getf(i) - ffc_mask.getf(i) * 2.8)); // Flat field correction
                    }
                }

                // Despeckle
                image::simple_despeckle(img, 20);
                image::append_ext(&extension);
                if (std::filesystem::exists(output_folder + "/" + filename + extension))
                {
                    int i = 0;
                    while (std::filesystem::exists(output_folder + "/" + filename + "-" + std::to_string(i) + extension))
                        i++;
                    filename = filename + "-" + std::to_string(i);
                }
                WRITE_IMAGE_LOCAL(img, output_folder + "/" + filename);
            }
        }
    } // namespace swap
} // namespace proba