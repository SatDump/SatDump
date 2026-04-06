#include "swap_reader.h"
#include "common/ccsds/ccsds_time.h"
#include "common/repack.h"
#include "core/resources.h"
#include "image/image.h"
#include "image/io.h"
#include "image/jpeg_utils.h"
#include "image/processing.h"
#include "logger.h"
#include "utils/binary.h"
#include <cstdint>
#include <ctime>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>

#define WRITE_IMAGE_LOCAL(img, path)                                                                                                                                                                   \
    {                                                                                                                                                                                                  \
        std::string newPath = path;                                                                                                                                                                    \
        image::append_ext(&newPath);                                                                                                                                                                   \
        image::save_img(img, std::string(newPath).c_str());                                                                                                                                            \
        all_images.push_back(newPath);                                                                                                                                                                 \
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
            if (packet.payload.size() < 8)
                return;

            time_t timestamp = ccsds::parseCCSDSTime(packet, 18630) + 4 * 3600;

            // Start new image
            if (currentOuts.find(timestamp) == currentOuts.end())
            {
                std::tm timeReadable = *gmtime(&timestamp);
                std::string timestamp_str = (timeReadable.tm_mday > 9 ? std::to_string(timeReadable.tm_mday) : "0" + std::to_string(timeReadable.tm_mday)) + "/" +
                                            (timeReadable.tm_mon + 1 > 9 ? std::to_string(timeReadable.tm_mon + 1) : "0" + std::to_string(timeReadable.tm_mon + 1)) + "/" +
                                            std::to_string(timeReadable.tm_year + 1900) + " " +
                                            (timeReadable.tm_hour > 9 ? std::to_string(timeReadable.tm_hour) : "0" + std::to_string(timeReadable.tm_hour)) + ":" +
                                            (timeReadable.tm_min > 9 ? std::to_string(timeReadable.tm_min) : "0" + std::to_string(timeReadable.tm_min)) + ":" +
                                            (timeReadable.tm_sec > 9 ? std::to_string(timeReadable.tm_sec) : "0" + std::to_string(timeReadable.tm_sec));
                logger->info("Found new SWAP image! Saving as SWAP-" + std::to_string(count) + ".jpeg. Timestamp " + timestamp_str);

                std::string utc_filename = "SWAP_" +                                                                                                                 // Instrument name
                                           std::to_string(timeReadable.tm_year + 1900) +                                                                             // Year yyyy
                                           (timeReadable.tm_mon + 1 > 9 ? std::to_string(timeReadable.tm_mon + 1) : "0" + std::to_string(timeReadable.tm_mon + 1)) + // Month MM
                                           (timeReadable.tm_mday > 9 ? std::to_string(timeReadable.tm_mday) : "0" + std::to_string(timeReadable.tm_mday)) + "T" +    // Day dd
                                           (timeReadable.tm_hour > 9 ? std::to_string(timeReadable.tm_hour) : "0" + std::to_string(timeReadable.tm_hour)) +          // Hour HH
                                           (timeReadable.tm_min > 9 ? std::to_string(timeReadable.tm_min) : "0" + std::to_string(timeReadable.tm_min)) +             // Minutes mm
                                           (timeReadable.tm_sec > 9 ? std::to_string(timeReadable.tm_sec) : "0" + std::to_string(timeReadable.tm_sec)) + "Z";        // Seconds ss

                currentOuts.insert({timestamp, WIP_Swap{utc_filename}});
                count++;
            }

            auto &wip = currentOuts[timestamp];
            try
            {
                wip.payload_jpg.insert(wip.payload_jpg.end(), &packet.payload[wip.nsegs == 0 ? 14 + 78 : 14], &packet.payload.back());
                packet.payload.resize(65530);
                wip.payload_raw.insert(wip.payload_raw.end(), &packet.payload[8], &packet.payload.back() - 1);
            }
            catch (std::exception &)
            {
            }
            wip.nsegs++;
        }

        void SWAPReader::save()
        {
            // This is temporary code until a resource system is implemented everywhere.
            /*image::Image adc_mask, ffc_mask;
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
            }*/

            for (auto &cswap : currentOuts)
            {
                std::string extension = "";
                std::string filename = cswap.second.filename;

                logger->info("Decompressing " + filename + "...");

                image::Image img = image::decompress_jpeg(cswap.second.payload_jpg.data(), cswap.second.payload_jpg.size());

                if (img.size() == 0)
                {
                    logger->info("Error! Skipping...");

#if 1
                    auto &pp = cswap.second.payload_raw;

                    for (int pos = 0; pos < pp.size() - 12; pos += 12)
                    {
                        uint8_t v[12];
                        for (int i = 0; i < 12; i++)
                            v[i] = pp[pos + i];

                        // There may be a bug here!
                        pp[pos + 9] = v[4];
                        pp[pos + 10] = v[5];
                        pp[pos + 11] = v[6];
                        pp[pos + 0] = v[7];
                        pp[pos + 1] = v[0];
                        pp[pos + 2] = v[1];
                        pp[pos + 3] = v[2];
                        pp[pos + 4] = v[3];
                        pp[pos + 5] = v[8];
                        pp[pos + 6] = v[9];
                        pp[pos + 7] = v[10];
                        pp[pos + 8] = v[11];
                    }
#endif

                    img.init(16, 1024, 1024, 1);
                    pp.resize((1024 * 1024 * 12) / 8);
                    repackBytesTo12bits(pp.data(), pp.size(), (uint16_t *)img.raw_data());

                    for (size_t s = 0; s < 1024 * 1024; s++)
                        img.set(s, img.get(s) << 4);

                    for (size_t s = 0; s < 1024 * 1024; s += 4)
                    {
                        uint16_t p[4];
                        p[0] = img.get(s + 0);
                        p[1] = img.get(s + 1);
                        p[2] = img.get(s + 2);
                        p[3] = img.get(s + 3);

                        img.set(s + 0, p[3]);
                        img.set(s + 1, p[2]);
                        img.set(s + 2, p[1]);
                        img.set(s + 3, p[0]);
                    }

                    filename += "_RAW";

                    // std::ofstream(output_folder + "/" + filename + ".bin", std::ios::binary).write((char *)pp.data(), pp.size());

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
                else
                {
                    /*if (masks_found)
                    {
                        for (size_t i = 0; i < img.height() * img.width(); i++)
                        {
                            // This was checked against official Proba-2 data
                            img.setf(i, std::max<float>(0, img.getf(i) - adc_mask.getf(i) * 2.8)); // ADC Bias correction
                            img.setf(i, std::max<float>(0, img.getf(i) - ffc_mask.getf(i) * 2.8)); // Flat field correction
                        }
                    }*/

                    // Despeckle
                    // image::simple_despeckle(img, 20);

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
        }
    } // namespace swap
} // namespace proba