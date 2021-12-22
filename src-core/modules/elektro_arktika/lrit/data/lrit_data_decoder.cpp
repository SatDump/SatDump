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
#include "common/utils.h"
#include "libs/others/strptime.h"
#include "imgui/imgui_image.h"
#include "common/image/jpeg_utils.h"
#include "resources.h"
#include "libs/DecompWT/CompressWT.h"
#include "libs/DecompWT/CompressT4.h"

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
                    header_parsed = false;
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

            if (file_in_progress && !header_parsed)
            {
                PrimaryHeader primary_header(&lrit_data[0]);
                header_parsed = true;

                if (lrit_data.size() >= primary_header.total_header_length)
                    parseHeader();
            }
        }

        void LRITDataDecoder::processLRITHeader(ccsds::CCSDSPacket &pkt)
        {
            lrit_data.insert(lrit_data.end(), &pkt.payload.data()[10], &pkt.payload.data()[pkt.payload.size() - 2]);
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

                    if (image_structure_record.compression_flag == 2 /* Progressive JPEG */)
                    {
                        logger->debug("JPEG Compression is used, decompressing...");
                        is_jpeg_compressed = true;
                        is_wt_compressed = false;
                    }
                    else if (image_structure_record.compression_flag == 1 /* Wavelet */)
                    {
                        logger->debug("Wavelet Compression is used, decompressing...");
                        is_jpeg_compressed = false;
                        is_wt_compressed = true;
                    }
                    else
                    {
                        is_jpeg_compressed = false;
                        is_wt_compressed = false;
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
                    image::Image<uint8_t> img = image::decompress_jpeg(&lrit_data[primary_header.total_header_length], lrit_data.size() - primary_header.total_header_length, true);
                    lrit_data.erase(lrit_data.begin() + primary_header.total_header_length, lrit_data.end());
                    lrit_data.insert(lrit_data.end(), (uint8_t *)&img[0], (uint8_t *)&img[img.height() * img.width()]);
                }
                else if (is_wt_compressed) // Is this Wavelet-Compressed? Decompress. We know this will always be 10-bits
                {
                    /*
                    This could be better... Sometimes I should maybe adapt the DecompWT code to be cleaner for this purpose...
                    */
                    logger->info("Decompressing Wavelet...");

                    int compression_type = 3; // We assume Wavelet

                    if (all_headers.count(SegmentIdentificationHeader::TYPE) > 0) // But if we have a header with better info, use it
                    {
                        SegmentIdentificationHeader segment_id_header(&lrit_data[all_headers[SegmentIdentificationHeader::TYPE]]);
                        compression_type = segment_id_header.compression;
                    }

                    // Init variables
                    uint8_t *image_ptr = NULL;
                    int buf_size = 0;
                    int out_pixels = 0;

                    // Decompress
                    {
                        // We need to copy over that memory to its own buffer
                        uint8_t *compressedData = new uint8_t[lrit_data.size() - primary_header.total_header_length];
                        std::memcpy(compressedData, &lrit_data[primary_header.total_header_length], lrit_data.size() - primary_header.total_header_length);

                        // Images object
                        Util::CDataFieldCompressedImage compressedImage(compressedData,
                                                                        (lrit_data.size() - primary_header.total_header_length) * 8,
                                                                        image_structure_record.bit_per_pixel,
                                                                        image_structure_record.columns_count,
                                                                        image_structure_record.lines_count);
                        Util::CDataFieldUncompressedImage decompressedImage;

                        // Perform WT Decompression
                        std::vector<short> m_QualityInfo;
                        if (compression_type == 3)
                            COMP::DecompressWT(compressedImage, image_structure_record.bit_per_pixel, decompressedImage, m_QualityInfo); // Standard HRIT compression
                        else if (compression_type == 2)
                            COMP::DecompressT4(compressedImage, decompressedImage, m_QualityInfo); // Shouldn't happen but better be ready
                        else
                            logger->error("Unknown compression! This should not have happened!");

                        // Fill our output buffer
                        buf_size = decompressedImage.Size() / 8;
                        out_pixels = decompressedImage.Size() / image_structure_record.bit_per_pixel;
                        image_ptr = new uint8_t[buf_size];
                        decompressedImage.Read(0, image_ptr, buf_size);
                    }

                    if (out_pixels == image_structure_record.columns_count * image_structure_record.lines_count) // Matches?
                    {
                        // Empty current LRIT file
                        lrit_data.erase(lrit_data.begin() + primary_header.total_header_length, lrit_data.end());

                        if (image_structure_record.bit_per_pixel == 10)
                        {
                            // Fill it back up converting to 8-bits
                            for (int i = 0; i < buf_size - (buf_size % 5); i += 5)
                            {
                                uint16_t v1 = (image_ptr[0] << 2) | (image_ptr[1] >> 6);
                                uint16_t v2 = ((image_ptr[1] % 64) << 4) | (image_ptr[2] >> 4);
                                uint16_t v3 = ((image_ptr[2] % 16) << 6) | (image_ptr[3] >> 2);
                                uint16_t v4 = ((image_ptr[3] % 4) << 8) | image_ptr[4];

                                lrit_data.push_back(v1 >> 2);
                                lrit_data.push_back(v2 >> 2);
                                lrit_data.push_back(v3 >> 2);
                                lrit_data.push_back(v4 >> 2);

                                image_ptr += 5;
                            }

                            // Fill remaining if required
                            for (int i = 0; i < buf_size % 5; i++)
                                lrit_data.push_back(0);
                        }
                        else if (image_structure_record.bit_per_pixel == 8) // Just in case
                        {
                            lrit_data.insert(lrit_data.end(), image_ptr, &image_ptr[buf_size]);
                        }
                        else
                        {
                            logger->error("Bit per pixels were not what they should!");
                        }
                    }
                    else // Fill it up with 0s
                    {
                        logger->error("Decompression result did not match image header. Discarding.");

                        out_pixels = image_structure_record.lines_count * image_structure_record.columns_count;

                        // Empty current LRIT file
                        lrit_data.erase(lrit_data.begin() + primary_header.total_header_length, lrit_data.end());

                        // Fill with 0s
                        for (int i = 0; i < out_pixels; i++)
                            lrit_data.push_back(0);
                    }

                    // Free up memory if necessary
                    //if (image_ptr != NULL)
                    //    delete[] image_ptr;
                }

                if (all_headers.count(SegmentIdentificationHeader::TYPE) > 0)
                {
                    imageStatus = RECEIVING;
                    SegmentIdentificationHeader segment_id_header(&lrit_data[all_headers[SegmentIdentificationHeader::TYPE]]);

                    std::vector<std::string> header_parts = splitString(current_filename, '_');

                    //for (std::string part : header_parts)
                    //    logger->trace(part);

                    std::string image_id = current_filename.substr(0, 30);

                    // Channel
                    int channel = segment_id_header.channel_id + 1;

                    // Timestamp
                    std::string timestamp = current_filename.substr(46, 12);
                    std::tm scanTimestamp;
                    strptime(timestamp.c_str(), "%Y%m%d%H%M", &scanTimestamp);
                    scanTimestamp.tm_sec = 0; // No seconds

                    // If we can, use a better filename
                    {
                        std::string sat_name = current_filename.substr(6, 5);

                        if (sat_name == "GOMS1") // Dead... But good measure
                        {
                            image_id = getHRITImageFilename(&scanTimestamp, "L1", channel);
                            elektro_221_composer_full_disk->filename = getHRITImageFilename(&scanTimestamp, "L1", "221");
                            elektro_321_composer_full_disk->filename321 = getHRITImageFilename(&scanTimestamp, "L1", "321");
                            elektro_321_composer_full_disk->filename231 = getHRITImageFilename(&scanTimestamp, "L1", "231");
                            elektro_321_composer_full_disk->filenameNC = getHRITImageFilename(&scanTimestamp, "L1", "NC");
                        }
                        else if (sat_name == "GOMS2")
                        {
                            image_id = getHRITImageFilename(&scanTimestamp, "L2", channel);
                            elektro_221_composer_full_disk->filename = getHRITImageFilename(&scanTimestamp, "L2", "221");
                            elektro_321_composer_full_disk->filename321 = getHRITImageFilename(&scanTimestamp, "L2", "321");
                            elektro_321_composer_full_disk->filename231 = getHRITImageFilename(&scanTimestamp, "L2", "231");
                            elektro_321_composer_full_disk->filenameNC = getHRITImageFilename(&scanTimestamp, "L2", "NC");
                        }
                        else if (sat_name == "GOMS3")
                        {
                            image_id = getHRITImageFilename(&scanTimestamp, "L3", channel);
                            elektro_221_composer_full_disk->filename = getHRITImageFilename(&scanTimestamp, "L3", "221");
                            elektro_321_composer_full_disk->filename321 = getHRITImageFilename(&scanTimestamp, "L3", "321");
                            elektro_321_composer_full_disk->filename231 = getHRITImageFilename(&scanTimestamp, "L3", "231");
                            elektro_321_composer_full_disk->filenameNC = getHRITImageFilename(&scanTimestamp, "L3", "NC");
                        }
                        else if (sat_name == "GOMS4") // Not launched yet, but we can expect it to be the same anyway
                        {
                            image_id = getHRITImageFilename(&scanTimestamp, "L3", channel);
                            elektro_221_composer_full_disk->filename = getHRITImageFilename(&scanTimestamp, "L4", "221");
                            elektro_321_composer_full_disk->filename321 = getHRITImageFilename(&scanTimestamp, "L4", "321");
                            elektro_321_composer_full_disk->filename231 = getHRITImageFilename(&scanTimestamp, "L4", "231");
                            elektro_321_composer_full_disk->filenameNC = getHRITImageFilename(&scanTimestamp, "L4", "NC");
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

                        segmentedDecoder = SegmentedLRITImageDecoder(segment_id_header.planned_end_segment,
                                                                     image_structure_record.columns_count,
                                                                     image_structure_record.lines_count,
                                                                     image_id);
                    }

                    int seg_number = segment_id_header.segment_sequence_number - 1;
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
                        image::Image<uint8_t> imageScaled = segmentedDecoder.image;
                        imageScaled.resize(img_width, img_height);
                        uchar_to_rgba(imageScaled.data(), textureBuffer, img_height * img_width);
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
                    image::Image<uint8_t> image(&lrit_data[primary_header.total_header_length], image_structure_record.columns_count, image_structure_record.lines_count, 1);
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