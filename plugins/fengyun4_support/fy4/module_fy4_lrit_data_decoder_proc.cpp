#include "module_fy4_lrit_data_decoder.h"
#include "logger.h"
#include "lrit_header.h"
#include <fstream>
#include "common/image/jpeg_utils.h"
#include "imgui/imgui_image.h"
#include "common/image/j2k_utils.h"
#include <filesystem>
#include "common/image/io.h"

namespace fy4
{
    namespace lrit
    {
        std::string getHRITImageFilename(std::tm *timeReadable, std::string sat_name, int channel)
        {
            std::string utc_filename = sat_name + "_" + std::to_string(channel) + "_" +                                                                             // Satellite name and channel
                                       std::to_string(timeReadable->tm_year + 1900) +                                                                               // Year yyyy
                                       (timeReadable->tm_mon + 1 > 9 ? std::to_string(timeReadable->tm_mon + 1) : "0" + std::to_string(timeReadable->tm_mon + 1)) + // Month MM
                                       (timeReadable->tm_mday > 9 ? std::to_string(timeReadable->tm_mday) : "0" + std::to_string(timeReadable->tm_mday)) + "T" +    // Day dd
                                       (timeReadable->tm_hour > 9 ? std::to_string(timeReadable->tm_hour) : "0" + std::to_string(timeReadable->tm_hour)) +          // Hour HH
                                       (timeReadable->tm_min > 9 ? std::to_string(timeReadable->tm_min) : "0" + std::to_string(timeReadable->tm_min)) +             // Minutes mm
                                       (timeReadable->tm_sec > 9 ? std::to_string(timeReadable->tm_sec) : "0" + std::to_string(timeReadable->tm_sec)) + "Z";        // Seconds ss
            return utc_filename;
        }

        void FY4LRITDataDecoderModule::processLRITFile(::lrit::LRITFile &file)
        {
            std::string current_filename = file.filename;

            ::lrit::PrimaryHeader primary_header = file.getHeader<::lrit::PrimaryHeader>();

            if (file.custom_flags[IS_ENCRYPTED]) // We lack decryption
            {
                if (!std::filesystem::exists(directory + "/LRIT_ENCRYPTED"))
                    std::filesystem::create_directory(directory + "/LRIT_ENCRYPTED");

                logger->info("Writing file " + directory + "/LRIT_ENCRYPTED/" + file.filename + "...");

                // Write file out
                std::ofstream fileo(directory + "/LRIT_ENCRYPTED/" + file.filename, std::ios::binary);
                fileo.write((char *)file.lrit_data.data(), file.lrit_data.size());
                fileo.close();
            }
            else
            {
                if (primary_header.file_type_code == 0 && file.hasHeader<ImageInformationRecord>())
                {
                    std::vector<std::string> header_parts = splitString(current_filename, '_');

                    // for (std::string part : header_parts)
                    //     logger->trace(part);

                    ImageInformationRecord image_structure_record = file.getHeader<ImageInformationRecord>();

                    if (true) // file.custom_flags[JPG_COMPRESSED] || file.custom_flags[J2K_COMPRESSED]) // Is this Jpeg-Compressed? Decompress
                    {
                        logger->info("Decompressing J2K...");

                        int offset = 0;
                        {
                            uint8_t shifter[4] = {0, 0, 0, 0};
                            for (int i = primary_header.total_header_length; i < (int)file.lrit_data.size(); i++)
                            {
                                shifter[0] = shifter[1];
                                shifter[1] = shifter[2];
                                shifter[2] = shifter[3];
                                shifter[3] = file.lrit_data[i];

                                if (shifter[0] == 0xFF && shifter[1] == 0x4F && shifter[2] == 0xFF && shifter[3] == 0x51)
                                {
                                    offset = i - primary_header.total_header_length - 3;
                                    break;
                                }
                            }
                        }

                        image::Image img2 = image::decompress_j2k_openjp2(&file.lrit_data[primary_header.total_header_length + offset],
                                                                          file.lrit_data.size() - primary_header.total_header_length - offset);

                        // Rely on the background for bit depth,
                        // as apparently nothing else works expected.
                        int max_val = 0;
                        for (int i = 0; i < (int)img2.size(); i++)
                            if (img2.get(i) > max_val)
                                max_val = img2.get(i);

                        if (img2.depth() != 8)
                            img2 = img2.to8bits();
                        image::Image img = img2;
                        if (max_val == 255) // LRIT, 4-bits
                        {
                            for (int i = 0; i < (int)img.size(); i++)
                            {
                                int v = img2.get(i) << 4;
                                if (v > 255)
                                    v = 255;
                                img.set(i, v);
                            }
                        }
                        else // HRIT full 10-bits
                        {
                            for (int i = 0; i < (int)img.size(); i++)
                            {
                                int v = img2.get(i) >> 4;
                                if (v > 255)
                                    v = 255;
                                img.set(i, v);
                            }
                        }

                        if (img.width() < image_structure_record.columns_count || img.height() < image_structure_record.lines_count)
                            img.init(8, image_structure_record.columns_count, image_structure_record.lines_count, 1); // Just in case it's corrupted!

                        if (img.depth() != 8)
                            logger->error("ELEKTRO xRIT JPEG Depth should be 8!");

                        file.lrit_data.erase(file.lrit_data.begin() + primary_header.total_header_length, file.lrit_data.end());
                        file.lrit_data.insert(file.lrit_data.end(), (uint8_t *)img.raw_data(), (uint8_t *)img.raw_data() + img.height() * img.width());
                    }

                    if (!std::filesystem::exists(directory + "/IMAGES"))
                        std::filesystem::create_directory(directory + "/IMAGES");

                    int channel = image_structure_record.channel_number;

                    // Timestamp
                    std::string timestamp = header_parts[10];
                    std::tm scanTimestamp;
                    strptime(timestamp.c_str(), "%Y%m%d%H%M", &scanTimestamp);
                    scanTimestamp.tm_sec = 0; // No seconds

                    std::string orig_filename = current_filename;
                    std::string image_id = getHRITImageFilename(&scanTimestamp, image_structure_record.satellite_name.substr(0, 4), channel);

                    if (header_parts[3] == "DISK") // Segmented
                    {
                        if (all_wip_images.count(channel) == 0)
                            all_wip_images.insert({channel, std::make_unique<wip_images>()});

                        std::unique_ptr<wip_images> &wip_img = all_wip_images[channel];

                        if (segmentedDecoders.count(channel) <= 0)
                        {
                            segmentedDecoders.insert(std::pair<int, SegmentedLRITImageDecoder>(channel, SegmentedLRITImageDecoder()));
                            wip_img->imageStatus = IDLE;
                            wip_img->img_height = 0;
                            wip_img->img_width = 0;
                        }

                        wip_img->imageStatus = RECEIVING;

                        SegmentedLRITImageDecoder &segmentedDecoder = segmentedDecoders[channel];

                        if (segmentedDecoder.image_id != image_id)
                        {
                            if (segmentedDecoder.image_id != "")
                            {
                                current_filename = image_id;

                                wip_img->imageStatus = SAVING;
                                image::save_img(segmentedDecoder.image, directory + "/IMAGES/" + current_filename);
                                wip_img->imageStatus = RECEIVING;
                            }

                            segmentedDecoder = SegmentedLRITImageDecoder(image_structure_record.total_segment_count,
                                                                         image_structure_record.columns_count,
                                                                         image_structure_record.lines_count,
                                                                         image_id);
                        }

                        segmentedDecoder.pushSegment(&file.lrit_data[primary_header.total_header_length], image_structure_record.current_segment_number - 1, image_structure_record.lines_count);

                        // If the UI is active, update texture
                        if (wip_img->textureID > 0)
                        {
                            // Downscale image
                            wip_img->img_height = 1000;
                            wip_img->img_width = 1000;
                            image::Image imageScaled = segmentedDecoder.image;
                            imageScaled.resize(wip_img->img_width, wip_img->img_height);
                            image::image_to_rgba(imageScaled, wip_img->textureBuffer);
                            wip_img->hasToUpdate = true;
                        }

                        if (segmentedDecoder.isComplete())
                        {
                            current_filename = image_id;

                            wip_img->imageStatus = SAVING;
                            image::save_img(segmentedDecoder.image, directory + "/IMAGES/" + current_filename);
                            segmentedDecoder = SegmentedLRITImageDecoder();
                            wip_img->imageStatus = IDLE;
                        }
                    }
                    else
                    {
                        std::string img_type = header_parts[3] + "_" + std::to_string(image_structure_record.current_segment_pos);

                        if (!std::filesystem::exists(directory + "/IMAGES/" + img_type))
                            std::filesystem::create_directory(directory + "/IMAGES/" + img_type);

                        image::Image image(&file.lrit_data[primary_header.total_header_length], 8, image_structure_record.columns_count, image_structure_record.lines_count, 1);
                        image::save_img(image, directory + "/IMAGES/" + img_type + "/" + image_id);
                    }
                }
                else
                {
                    if (!std::filesystem::exists(directory + "/LRIT"))
                        std::filesystem::create_directory(directory + "/LRIT");

                    logger->info("Writing file " + directory + "/LRIT/" + file.filename + "...");

                    // Write file out
                    std::ofstream fileo(directory + "/LRIT/" + file.filename, std::ios::binary);
                    fileo.write((char *)file.lrit_data.data(), file.lrit_data.size());
                    fileo.close();
                }
            }
        }
    } // namespace avhrr
} // namespace metop