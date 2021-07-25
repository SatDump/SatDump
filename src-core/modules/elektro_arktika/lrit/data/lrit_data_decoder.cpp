#include "lrit_data_decoder.h"
#include "logger.h"
#include <fstream>
#include "lrit_header.h"
#include "crc_table.h"
#include <sstream>
#include <string>
#include <iomanip>
#include <filesystem>
#include <algorithm>
#include "common/miniz/miniz.h"
#include "common/miniz/miniz_zip.h"
#include "common/utils.h"
#include "common/others/strptime.h"
#include "imgui/imgui_image.h"
#include "common/image/image.h"
#include "resources.h"

namespace elektro
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

        std::string getHRITImageFilename(std::tm *timeReadable, std::string sat_name, std::string channel)
        {
            std::string utc_filename = sat_name + "_" + channel + "_" +                                                                                             // Satellite name and channel
                                       std::to_string(timeReadable->tm_year + 1900) +                                                                               // Year yyyy
                                       (timeReadable->tm_mon + 1 > 9 ? std::to_string(timeReadable->tm_mon + 1) : "0" + std::to_string(timeReadable->tm_mon + 1)) + // Month MM
                                       (timeReadable->tm_mday > 9 ? std::to_string(timeReadable->tm_mday) : "0" + std::to_string(timeReadable->tm_mday)) + "T" +    // Day dd
                                       (timeReadable->tm_hour > 9 ? std::to_string(timeReadable->tm_hour) : "0" + std::to_string(timeReadable->tm_hour)) +          // Hour HH
                                       (timeReadable->tm_min > 9 ? std::to_string(timeReadable->tm_min) : "0" + std::to_string(timeReadable->tm_min)) +             // Minutes mm
                                       (timeReadable->tm_sec > 9 ? std::to_string(timeReadable->tm_sec) : "0" + std::to_string(timeReadable->tm_sec)) + "Z";        // Seconds ss
            return utc_filename;
        }

        // CRC Implementation from LRIT-Missin-Specific-Document.pdf
        uint16_t computeCRC(const uint8_t *data, int size)
        {
            uint16_t crc = 0xffff;
            for (int i = 0; i < size; i++)
                crc = (crc << 8) ^ crc_table[(crc >> 8) ^ (uint16_t)data[i]];
            return crc;
        }

        LRITDataDecoder::LRITDataDecoder(std::string dir) : directory(dir)
        {
            file_in_progress = false;
            imageStatus = IDLE;
            img_height = 0;
            img_width = 0;
        }

        LRITDataDecoder::~LRITDataDecoder()
        {
        }

        void LRITDataDecoder::work(ccsds::CCSDSPacket &packet)
        {
            if (packet.header.sequence_flag == 1 || packet.header.sequence_flag == 3)
            {
                if (file_in_progress)
                    finalizeLRITData();

                lrit_data.clear();

                // Check CRC
                uint16_t crc = packet.payload.data()[packet.payload.size() - 2] << 8 | packet.payload.data()[packet.payload.size() - 1];

                if (crc == computeCRC(packet.payload.data(), packet.payload.size() - 2))
                {
                    processLRITHeader(packet);
                    file_in_progress = true;
                }
                else
                {
                    logger->error("CRC is invalid... Skipping.");
                    file_in_progress = false;
                }
            }
            else if (packet.header.sequence_flag == 0)
            {
                if (file_in_progress)
                {
                    processLRITData(packet);
                }
            }
            else if (packet.header.sequence_flag == 2)
            {
                if (file_in_progress)
                {
                    processLRITData(packet);
                    finalizeLRITData();
                    file_in_progress = false;
                }
            }
        }

        void LRITDataDecoder::processLRITHeader(ccsds::CCSDSPacket &pkt)
        {
            lrit_data.insert(lrit_data.end(), &pkt.payload.data()[10], &pkt.payload.data()[pkt.payload.size() - 2]);
            parseHeader();
        }

        void LRITDataDecoder::parseHeader()
        {
            PrimaryHeader primary_header(&lrit_data[0]);

            // Get all other headers
            all_headers.clear();
            for (uint32_t i = 0; i < primary_header.total_header_length;)
            {
                uint8_t type = lrit_data[i];
                uint16_t record_length = lrit_data[i + 1] << 8 | lrit_data[i + 2];

                if (record_length == 0)
                    break;

                all_headers.emplace(std::pair<int, int>(type, i));

                i += record_length;
            }

            // Check if this has a filename
            if (all_headers.count(AnnotationRecord::TYPE) > 0)
            {
                AnnotationRecord annotation_record(&lrit_data[all_headers[AnnotationRecord::TYPE]]);

                current_filename = std::string(annotation_record.annotation_text.data());

                std::replace(current_filename.begin(), current_filename.end(), '/', '_');  // Safety
                std::replace(current_filename.begin(), current_filename.end(), '\\', '_'); // Safety

                for (char &c : current_filename) // Strip invalid chars
                {
                    if (c < 33)
                        c = '_';
                }

                logger->info("New LRIT file : " + current_filename);

                // Check if this is image data
                if (all_headers.count(ImageStructureRecord::TYPE) > 0)
                {
                    ImageStructureRecord image_structure_record(&lrit_data[all_headers[ImageStructureRecord::TYPE]]);
                    logger->debug("This is image data. Size " + std::to_string(image_structure_record.columns_count) + "x" + std::to_string(image_structure_record.lines_count));

                    if (image_structure_record.compression_flag == 2 /* Progressive JPEG */ || image_structure_record.compression_flag == 1 /* JPEG 2000 */)
                    {
                        logger->debug("JPEG Compression is used, decompressing...");
                        is_jpeg_compressed = true;
                    }
                    else
                    {
                        is_jpeg_compressed = false;
                    }
                }

                if (all_headers.count(KeyHeader::TYPE) > 0)
                {
                    KeyHeader key_header(&lrit_data[all_headers[KeyHeader::TYPE]]);
                    logger->debug("This is encrypted!");
                }
            }
        }

        void LRITDataDecoder::processLRITData(ccsds::CCSDSPacket &pkt)
        {
            // File may be encrypted so we cannot process them before the fact...
            lrit_data.insert(lrit_data.end(), &pkt.payload.data()[0], &pkt.payload.data()[pkt.payload.size() - 2]);
        }

        void LRITDataDecoder::finalizeLRITData()
        {
            PrimaryHeader primary_header(&lrit_data[0]);

            // Check if this is image data, and if so also write it as an image
            if (primary_header.file_type_code == 0 && all_headers.count(ImageStructureRecord::TYPE) > 0)
            {
                if (!std::filesystem::exists(directory + "/IMAGES"))
                    std::filesystem::create_directory(directory + "/IMAGES");

                ImageStructureRecord image_structure_record(&lrit_data[all_headers[ImageStructureRecord::TYPE]]);

                if (is_jpeg_compressed) // Is this Jpeg-Compressed? Decompress
                {
                    logger->info("Decompressing JPEG...");
                    cimg_library::CImg<unsigned char> img = image::decompress_jpeg(&lrit_data[primary_header.total_header_length], lrit_data.size() - primary_header.total_header_length, true);
                    lrit_data.erase(lrit_data.begin() + primary_header.total_header_length, lrit_data.end());
                    lrit_data.insert(lrit_data.end(), (uint8_t *)&img[0], (uint8_t *)&img[img.height() * img.width()]);
                }

                if (all_headers.count(SegmentIdentificationHeader::TYPE) > 0)
                {
                    imageStatus = RECEIVING;

                    std::vector<std::string> header_parts = splitString(current_filename, '_');

                    //for (std::string part : header_parts)
                    //    logger->trace(part);

                    std::string image_id = current_filename.substr(0, 30);

                    // Channel
                    int channel = std::stoi(current_filename.substr(29, 1));
                    {
                        int aux_ch = std::stoi(current_filename.substr(26, 2));
                        if (aux_ch > 0)
                            channel += aux_ch;

                        // Now, we remap the channel to the "normal" MSU-GS numbering
                        if (channel == 6) // 6 is 1
                            channel = 1;
                        else if (channel == 7) // 7 is 2
                            channel = 2;
                        else if (channel == 9) // 9 is 3
                            channel = 3;
                        else // May for IR?
                            channel = aux_ch - channel;
                    }

                    // Timestamp
                    std::string timestamp = current_filename.substr(46, 12);
                    std::tm scanTimestamp;
                    strptime(timestamp.c_str(), "%Y%m%d%H%M", &scanTimestamp);
                    scanTimestamp.tm_sec = 0; // No seconds

                    // If we can, use a better filename
                    {
                        std::string product_name = current_filename.substr(0, 18);

                        if (product_name == "L-000-GOMS2_-GOMS2")
                        {
                            image_id = getHRITImageFilename(&scanTimestamp, "L2", channel);
                            elektro_221_composer_full_disk->filename = getHRITImageFilename(&scanTimestamp, "L2", "221");
                            elektro_321_composer_full_disk->filename321 = getHRITImageFilename(&scanTimestamp, "L2", "321");
                            elektro_321_composer_full_disk->filename231 = getHRITImageFilename(&scanTimestamp, "L2", "231");
                        }

                        if (product_name == "L-000-GOMS3_-GOMS3")
                        {
                            image_id = getHRITImageFilename(&scanTimestamp, "L3", channel);
                            elektro_221_composer_full_disk->filename = getHRITImageFilename(&scanTimestamp, "L3", "221");
                            elektro_321_composer_full_disk->filename321 = getHRITImageFilename(&scanTimestamp, "L3", "321");
                            elektro_321_composer_full_disk->filename231 = getHRITImageFilename(&scanTimestamp, "L3", "231");
                        }
                    }

                    if (segmentedDecoder.image_id != image_id)
                    {
                        if (segmentedDecoder.image_id != "")
                        {
                            current_filename = image_id;

                            imageStatus = SAVING;
                            logger->info("Writing image " + directory + "/IMAGES/" + current_filename + ".png" + "...");
                            segmentedDecoder.image.save_png(std::string(directory + "/IMAGES/" + current_filename + ".png").c_str());

                            if (elektro_221_composer_full_disk->hasData)
                                elektro_221_composer_full_disk->save(directory);
                            if (elektro_321_composer_full_disk->hasData)
                                elektro_321_composer_full_disk->save(directory);

                            imageStatus = RECEIVING;
                        }

                        segmentedDecoder = SegmentedLRITImageDecoder(6,
                                                                     image_structure_record.columns_count,
                                                                     image_structure_record.lines_count,
                                                                     image_id);
                    }

                    std::vector<std::string> header_parts2;
                    if (header_parts.size() >= 10)
                        header_parts2 = splitString(header_parts[9], '-');

                    int seg_number = 0;

                    if (header_parts2.size() >= 2)
                    {
                        seg_number = std::stoi(header_parts2[1]) - 1;
                    }
                    else
                    {
                        logger->critical("Could not parse segment number from filename!");

                        // Fallback, maybe unreliable code
                        SegmentIdentificationHeader segment_id_header(&lrit_data[all_headers[SegmentIdentificationHeader::TYPE]]);
                        seg_number = (segment_id_header.segment_sequence_number - 1) / image_structure_record.lines_count;
                    }
                    segmentedDecoder.pushSegment(&lrit_data[primary_header.total_header_length], seg_number);

                    // Composite?
                    if (channel == 1)
                    {
                        elektro_221_composer_full_disk->push1(segmentedDecoder.image, mktime(&scanTimestamp));
                        elektro_321_composer_full_disk->push1(segmentedDecoder.image, mktime(&scanTimestamp));
                    }
                    else if (channel == 2)
                    {
                        elektro_221_composer_full_disk->push2(segmentedDecoder.image, mktime(&scanTimestamp));
                        elektro_321_composer_full_disk->push2(segmentedDecoder.image, mktime(&scanTimestamp));
                    }
                    else if (channel == 3)
                    {
                        elektro_321_composer_full_disk->push3(segmentedDecoder.image, mktime(&scanTimestamp));
                    }

                    // If the UI is active, update texture
                    if (textureID > 0)
                    {
                        // Downscale image
                        img_height = 1000;
                        img_width = 1000;
                        cimg_library::CImg<unsigned char> imageScaled = segmentedDecoder.image;
                        imageScaled.resize(img_width, img_height);
                        uchar_to_rgba(imageScaled, textureBuffer, img_height * img_width);
                        hasToUpdate = true;
                    }

                    if (segmentedDecoder.isComplete())
                    {
                        current_filename = image_id;

                        imageStatus = SAVING;
                        logger->info("Writing image " + directory + "/IMAGES/" + current_filename + ".png" + "...");
                        segmentedDecoder.image.save_png(std::string(directory + "/IMAGES/" + current_filename + ".png").c_str());

                        if (elektro_221_composer_full_disk->hasData)
                            elektro_221_composer_full_disk->save(directory);
                        if (elektro_321_composer_full_disk->hasData)
                            elektro_321_composer_full_disk->save(directory);

                        segmentedDecoder = SegmentedLRITImageDecoder();
                        imageStatus = IDLE;
                    }
                }
                else
                {
                    // Write raw image dats
                    logger->info("Writing image " + directory + "/IMAGES/" + current_filename + ".png" + "...");
                    cimg_library::CImg<unsigned char> image(&lrit_data[primary_header.total_header_length], image_structure_record.columns_count, image_structure_record.lines_count);
                    image.save_png(std::string(directory + "/IMAGES/" + current_filename + ".png").c_str());
                }
            }
            else
            {
                if (!std::filesystem::exists(directory + "/LRIT"))
                    std::filesystem::create_directory(directory + "/LRIT");

                logger->info("Writing file " + directory + "/LRIT/" + current_filename + "...");

                // Write file out
                std::ofstream file(directory + "/LRIT/" + current_filename, std::ios::binary);
                file.write((char *)lrit_data.data(), lrit_data.size());
                file.close();
            }
        }

        void LRITDataDecoder::save()
        {
            if (segmentedDecoder.image_id != "")
            {
                finalizeLRITData();

                logger->info("Writing image " + directory + "/IMAGES/" + current_filename + ".png" + "...");
                segmentedDecoder.image.save_png(std::string(directory + "/IMAGES/" + current_filename + ".png").c_str());
            }
        }
    } // namespace atms
} // namespace jpss