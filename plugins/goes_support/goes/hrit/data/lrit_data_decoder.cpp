#include "lrit_data_decoder.h"
#include "logger.h"
#include <fstream>
#include "lrit_header.h"
#include "crc_table.h"
#include <sstream>
#include <string>
#include <iomanip>
#include "products.h"
#include <filesystem>
#include <algorithm>
#include "libs/miniz/miniz.h"
#include "libs/miniz/miniz_zip.h"
#include "common/utils.h"
#include "libs/others/strptime.h"
#include "imgui/imgui_image.h"

namespace goes
{
    namespace hrit
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
            is_goesn = false;
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
                    is_goesn = false;
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

                // Taken from goestools... Took me a while to figure out what was going on there.. Damn it!
                /*NOAALRITHeader noaa_header(&lrit_data[all_headers[NOAALRITHeader::TYPE]]);
                if (primary_header.file_type_code == 0 && (noaa_header.product_id == 16 || noaa_header.product_id == 17))
                {
                    if (all_headers.count(SegmentIdentificationHeader::TYPE) > 0)
                    {
                        SegmentIdentificationHeader segment_id_header(&lrit_data[all_headers[SegmentIdentificationHeader::TYPE]]);
                        std::stringstream suffix;
                        suffix << "_" << std::setfill('0') << std::setw(3) << segment_id_header.segment_sequence_number;
                        current_filename.insert(current_filename.rfind(".lrit"), suffix.str());
                    }
                }*/

                logger->info("New LRIT file : " + current_filename);

                // Check if this is image data
                if (all_headers.count(ImageStructureRecord::TYPE) > 0)
                {
                    ImageStructureRecord image_structure_record(&lrit_data[all_headers[ImageStructureRecord::TYPE]]);
                    logger->debug("This is image data. Size " + std::to_string(image_structure_record.columns_count) + "x" + std::to_string(image_structure_record.lines_count));

                    NOAALRITHeader noaa_header(&lrit_data[all_headers[NOAALRITHeader::TYPE]]);

                    if (image_structure_record.compression_flag == 1 && noaa_header.noaa_specific_compression == 1) // Check this is RICE
                    {
                        is_rice_compressed = true;

                        logger->debug("This is RICE-compressed! Decompressing...");

                        rice_parameters.bits_per_pixel = image_structure_record.bit_per_pixel;
                        rice_parameters.pixels_per_block = 16 /* * 2*/; // The real default is 16, but has been to 32 for ages
                        rice_parameters.pixels_per_scanline = image_structure_record.columns_count;
                        rice_parameters.options_mask = SZ_ALLOW_K13_OPTION_MASK | SZ_MSB_OPTION_MASK | SZ_NN_OPTION_MASK | SZ_RAW_OPTION_MASK;

                        if (all_headers.count(RiceCompressionHeader::TYPE) > 0)
                        {
                            RiceCompressionHeader rice_compression_header(&lrit_data[all_headers[RiceCompressionHeader::TYPE]]);
                            logger->debug("Rice header is present!");
                            logger->debug("Pixels per block : " + std::to_string(rice_compression_header.pixels_per_block));
                            logger->debug("Scan lines per packet : " + std::to_string(rice_compression_header.scanlines_per_packet));
                            rice_parameters.pixels_per_block = rice_compression_header.pixels_per_block;
                            rice_parameters.options_mask = rice_compression_header.flags | SZ_RAW_OPTION_MASK;
                        }

                        decompression_buffer.resize(rice_parameters.pixels_per_scanline);
                    }
                    else
                    {
                        is_rice_compressed = false;
                    }
                }
            }
        }

        void LRITDataDecoder::processLRITData(ccsds::CCSDSPacket &pkt)
        {
            if (is_rice_compressed)
            {
                if (rice_parameters.bits_per_pixel == 0)
                    return;

                if (rice_parameters.pixels_per_block <= 0)
                {
                    logger->critical("Pixel per blocks is set to 0! Using defaults");
                    rice_parameters.pixels_per_block = 16 * 2; // The real default is 16, but has been to 32 for ages
                }

                size_t output_size = decompression_buffer.size();
                int r = SZ_BufftoBuffDecompress(decompression_buffer.data(), &output_size, pkt.payload.data(), pkt.payload.size(), &rice_parameters);
                if (r == AEC_OK)
                {
                    lrit_data.insert(lrit_data.end(), &decompression_buffer.data()[0], &decompression_buffer.data()[output_size]);
                }
                else
                {
                    logger->warn("Rice decompression failed. This may be an issue!");
                }
            }
            else
            {
                lrit_data.insert(lrit_data.end(), &pkt.payload.data()[0], &pkt.payload.data()[pkt.payload.size()]);
            }
        }

        void LRITDataDecoder::finalizeLRITData()
        {
            PrimaryHeader primary_header(&lrit_data[0]);
            NOAALRITHeader noaa_header(&lrit_data[all_headers[NOAALRITHeader::TYPE]]);

            // Check if this is image data, and if so also write it as an image
            if (write_images && primary_header.file_type_code == 0 && all_headers.count(ImageStructureRecord::TYPE) > 0)
            {
                if (!std::filesystem::exists(directory + "/IMAGES"))
                    std::filesystem::create_directory(directory + "/IMAGES");

                ImageStructureRecord image_structure_record(&lrit_data[all_headers[ImageStructureRecord::TYPE]]);

                TimeStampRecord timestamp_record(&lrit_data[all_headers[TimeStampRecord::TYPE]]);
                std::tm *timeReadable = gmtime(&timestamp_record.timestamp);

                std::string old_filename = current_filename;

                // Process as a specific dataset
                {
                    // GOES-R Data, from GOES-16 to 19.
                    // Once again peeked in goestools for the meso detection, sorry :-)
                    if (primary_header.file_type_code == 0 && (noaa_header.product_id == 16 ||
                                                               noaa_header.product_id == 17 ||
                                                               noaa_header.product_id == 18 ||
                                                               noaa_header.product_id == 19))
                    {
                        std::vector<std::string> cutFilename = splitString(current_filename, '-');

                        if (cutFilename.size() >= 4)
                        {
                            int mode = -1;
                            int channel = -1;

                            if (sscanf(cutFilename[3].c_str(), "M%dC%02d", &mode, &channel) == 2)
                            {
                                AncillaryTextRecord ancillary_record(&lrit_data[all_headers[AncillaryTextRecord::TYPE]]);

                                std::string region = "Others";

                                // Parse Region
                                if (ancillary_record.meta.count("Region") > 0)
                                {
                                    std::string regionName = ancillary_record.meta["Region"];

                                    if (regionName == "Full Disk")
                                    {
                                        region = "Full Disk";
                                    }
                                    else if (regionName == "Mesoscale")
                                    {
                                        if (cutFilename[2] == "CMIPM1")
                                            region = "Mesoscale 1";
                                        else if (cutFilename[2] == "CMIPM2")
                                            region = "Mesoscale 2";
                                        else
                                            region = "Mesoscale";
                                    }
                                }

                                // Parse scan time
                                std::tm scanTimestamp = *timeReadable;                      // Default to CCSDS timestamp normally...
                                if (ancillary_record.meta.count("Time of frame start") > 0) // ...unless we have a proper scan time
                                {
                                    std::string scanTime = ancillary_record.meta["Time of frame start"];
                                    strptime(scanTime.c_str(), "%Y-%m-%dT%H:%M:%S", &scanTimestamp);
                                }

                                std::string subdir = "GOES-" + std::to_string(noaa_header.product_id) + "/" + region;

                                if (!std::filesystem::exists(directory + "/IMAGES/" + subdir))
                                    std::filesystem::create_directories(directory + "/IMAGES/" + subdir);

                                current_filename = subdir + "/" + getHRITImageFilename(&scanTimestamp, "G" + std::to_string(noaa_header.product_id), channel);

                                if ((region == "Full Disk" || region == "Mesoscale 1" || region == "Mesoscale 2"))
                                {
                                    std::shared_ptr<GOESRFalseColorComposer> goes_r_fc_composer;

                                    if (region == "Full Disk")
                                    {
                                        goes_r_fc_composer = goes_r_fc_composer_full_disk;

                                        /*if (channel == 2)
                                        {
                                            goes_r_fc_composer->push2(segmentedDecoder.image, mktime(&scanTimestamp));
                                        }
                                        else if (channel == 13)
                                        {
                                            goes_r_fc_composer->push13(segmentedDecoder.image, mktime(&scanTimestamp));
                                        }*/
                                    }
                                    else if (region == "Mesoscale 1" || region == "Mesoscale 2")
                                    {
                                        if (region == "Mesoscale 1")
                                            goes_r_fc_composer = goes_r_fc_composer_meso1;
                                        else if (region == "Mesoscale 2")
                                            goes_r_fc_composer = goes_r_fc_composer_meso2;

                                        image::Image<uint8_t> image(&lrit_data[primary_header.total_header_length],
                                                                    image_structure_record.columns_count,
                                                                    image_structure_record.lines_count, 1);

                                        if (channel == 2)
                                        {
                                            goes_r_fc_composer->push2(image, mktime(&scanTimestamp));
                                        }
                                        else if (channel == 13)
                                        {
                                            goes_r_fc_composer->push13(image, mktime(&scanTimestamp));
                                        }
                                    }

                                    goes_r_fc_composer->filename = subdir + "/" + getHRITImageFilename(&scanTimestamp, "G" + std::to_string(noaa_header.product_id), "FC");
                                }
                            }
                        }
                    }
                    // GOES-N Data, from GOES-13 to 15.
                    else if (primary_header.file_type_code == 0 && (noaa_header.product_id == 13 ||
                                                                    noaa_header.product_id == 14 ||
                                                                    noaa_header.product_id == 15))
                    {
                        is_goesn = true;

                        int channel = -1;

                        // Parse channel
                        if (noaa_header.product_subid <= 10)
                            channel = 4;
                        else if (noaa_header.product_subid <= 20)
                            channel = 1;
                        else if (noaa_header.product_subid <= 30)
                            channel = 3;

                        std::string region = "Others";

                        // Parse Region. Had to peak in goestools again...
                        if (noaa_header.product_subid % 10 == 1)
                            region = "Full Disk";
                        else if (noaa_header.product_subid % 10 == 2)
                            region = "Northern Hemisphere";
                        else if (noaa_header.product_subid % 10 == 3)
                            region = "Southern Hemisphere";
                        else if (noaa_header.product_subid % 10 == 4)
                            region = "United States";
                        else
                        {
                            char buf[32];
                            size_t len;
                            int num = (noaa_header.product_subid % 10) - 5;
                            len = snprintf(buf, 32, "Special Interest %d", num);
                            region = std::string(buf, len);
                        }

                        // Parse scan time
                        AncillaryTextRecord ancillary_record(&lrit_data[all_headers[AncillaryTextRecord::TYPE]]);
                        std::tm scanTimestamp = *timeReadable;                      // Default to CCSDS timestamp normally...
                        if (ancillary_record.meta.count("Time of frame start") > 0) // ...unless we have a proper scan time
                        {
                            std::string scanTime = ancillary_record.meta["Time of frame start"];
                            strptime(scanTime.c_str(), "%Y-%m-%dT%H:%M:%S", &scanTimestamp);
                        }

                        std::string subdir = "GOES-" + std::to_string(noaa_header.product_id) + "/" + region;

                        if (!std::filesystem::exists(directory + "/IMAGES/" + subdir))
                            std::filesystem::create_directories(directory + "/IMAGES/" + subdir);

                        current_filename = subdir + "/" + getHRITImageFilename(&scanTimestamp, "G" + std::to_string(noaa_header.product_id), channel);
                    }
                    // Himawari-8 rebroadcast
                    else if (primary_header.file_type_code == 0 && noaa_header.product_id == 43)
                    {
                        std::string subdir = "Himawari-8/Full Disk";

                        if (!std::filesystem::exists(directory + "/IMAGES/" + subdir))
                            std::filesystem::create_directories(directory + "/IMAGES/" + subdir);

                        // Apparently the timestamp is in there for Himawari-8 data
                        AnnotationRecord annotation_record(&lrit_data[all_headers[AnnotationRecord::TYPE]]);

                        std::vector<std::string> strParts = splitString(annotation_record.annotation_text, '_');
                        if (strParts.size() > 3)
                        {
                            strptime(strParts[2].c_str(), "%Y%m%d%H%M", timeReadable);
                            current_filename = subdir + "/" + getHRITImageFilename(timeReadable, "HIM8", noaa_header.product_subid); // SubID = Channel
                        }
                    }
                    // NWS Images
                    else if (primary_header.file_type_code == 0 && noaa_header.product_id == 6)
                    {
                        std::string subdir = "NWS";

                        if (!std::filesystem::exists(directory + "/IMAGES/" + subdir))
                            std::filesystem::create_directories(directory + "/IMAGES/" + subdir);

                        std::string back = current_filename;
                        current_filename = subdir + "/" + back;
                    }
                }

                if (all_headers.count(SegmentIdentificationHeader::TYPE) > 0)
                {
                    imageStatus = RECEIVING;

                    SegmentIdentificationHeader segment_id_header(&lrit_data[all_headers[SegmentIdentificationHeader::TYPE]]);

                    if (segmentedDecoder.image_id != segment_id_header.image_identifier)
                    {
                        if (segmentedDecoder.image_id != -1)
                        {
                            imageStatus = SAVING;
                            logger->info("Writing image " + directory + "/IMAGES/" + current_filename + ".png" + "...");
                            if (is_goesn)
                                segmentedDecoder.image.resize(segmentedDecoder.image.width(), segmentedDecoder.image.height() * 1.75);
                            segmentedDecoder.image.save_png(std::string(directory + "/IMAGES/" + current_filename + ".png").c_str());
                            imageStatus = RECEIVING;

                            // Check if this is GOES-R
                            if (primary_header.file_type_code == 0 && (noaa_header.product_id == 16 ||
                                                                       noaa_header.product_id == 17 ||
                                                                       noaa_header.product_id == 18 ||
                                                                       noaa_header.product_id == 19))
                            {
                                int mode = -1;
                                int channel = -1;
                                std::vector<std::string> cutFilename = splitString(old_filename, '-');
                                if (cutFilename.size() > 3)
                                {
                                    if (sscanf(cutFilename[3].c_str(), "M%dC%02d", &mode, &channel) == 2)
                                    {
                                        if (goes_r_fc_composer_full_disk->hasData && (channel == 2 || channel == 2))
                                            goes_r_fc_composer_full_disk->save(directory);
                                    }
                                }
                            }
                        }

                        segmentedDecoder = SegmentedLRITImageDecoder(segment_id_header.max_segment,
                                                                     image_structure_record.columns_count,
                                                                     image_structure_record.lines_count,
                                                                     segment_id_header.image_identifier);
                    }

                    if (noaa_header.product_id == ID_HIMAWARI8)
                        segmentedDecoder.pushSegment(&lrit_data[primary_header.total_header_length], segment_id_header.segment_sequence_number - 1);
                    else
                        segmentedDecoder.pushSegment(&lrit_data[primary_header.total_header_length], segment_id_header.segment_sequence_number);

                    // Check if this is GOES-R, if yes, MS
                    if (primary_header.file_type_code == 0 && (noaa_header.product_id == 16 ||
                                                               noaa_header.product_id == 17 ||
                                                               noaa_header.product_id == 18 ||
                                                               noaa_header.product_id == 19))
                    {
                        int mode = -1;
                        int channel = -1;
                        std::vector<std::string> cutFilename = splitString(old_filename, '-');
                        if (cutFilename.size() > 3)
                        {
                            if (sscanf(cutFilename[3].c_str(), "M%dC%02d", &mode, &channel) == 2)
                            {
                                AncillaryTextRecord ancillary_record(&lrit_data[all_headers[AncillaryTextRecord::TYPE]]);

                                // Parse scan time
                                std::tm scanTimestamp = *timeReadable;                      // Default to CCSDS timestamp normally...
                                if (ancillary_record.meta.count("Time of frame start") > 0) // ...unless we have a proper scan time
                                {
                                    std::string scanTime = ancillary_record.meta["Time of frame start"];
                                    strptime(scanTime.c_str(), "%Y-%m-%dT%H:%M:%S", &scanTimestamp);
                                }

                                if (channel == 2)
                                    goes_r_fc_composer_full_disk->push2(segmentedDecoder.image, mktime(&scanTimestamp));
                                else if (channel == 13)
                                    goes_r_fc_composer_full_disk->push13(segmentedDecoder.image, mktime(&scanTimestamp));
                            }
                        }
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
                        imageStatus = SAVING;
                        logger->info("Writing image " + directory + "/IMAGES/" + current_filename + ".png" + "...");
                        if (is_goesn)
                            segmentedDecoder.image.resize(segmentedDecoder.image.width(), segmentedDecoder.image.height() * 1.75);
                        segmentedDecoder.image.save_png(std::string(directory + "/IMAGES/" + current_filename + ".png").c_str());
                        segmentedDecoder = SegmentedLRITImageDecoder();
                        imageStatus = IDLE;

                        // Check if this is GOES-R
                        if (primary_header.file_type_code == 0 && (noaa_header.product_id == 16 ||
                                                                   noaa_header.product_id == 17 ||
                                                                   noaa_header.product_id == 18 ||
                                                                   noaa_header.product_id == 19))
                        {
                            int mode = -1;
                            int channel = -1;
                            std::vector<std::string> cutFilename = splitString(old_filename, '-');
                            if (cutFilename.size() > 3)
                            {
                                if (sscanf(cutFilename[3].c_str(), "M%dC%02d", &mode, &channel) == 2)
                                {
                                    if (goes_r_fc_composer_full_disk->hasData && (channel == 2 || channel == 2))
                                        goes_r_fc_composer_full_disk->save(directory);
                                }
                            }
                        }
                    }
                }
                else
                {
                    if (noaa_header.noaa_specific_compression == 5) // Gif?
                    {
                        logger->info("Writing file " + directory + "/IMAGES/" + current_filename + ".gif" + "...");

                        int offset = primary_header.total_header_length;

                        // Write file out
                        std::ofstream file(directory + "/IMAGES/" + current_filename + ".gif");
                        file.write((char *)&lrit_data[offset], lrit_data.size() - offset);
                        file.close();
                    }
                    else // Write raw image dats
                    {
                        logger->info("Writing image " + directory + "/IMAGES/" + current_filename + ".png" + "...");
                        image::Image<uint8_t> image(&lrit_data[primary_header.total_header_length], image_structure_record.columns_count, image_structure_record.lines_count, 1);
                        if (is_goesn)
                            segmentedDecoder.image.resize(segmentedDecoder.image.width(), segmentedDecoder.image.height() * 1.75);
                        image.save_png(std::string(directory + "/IMAGES/" + current_filename + ".png").c_str());

                        // Check if this is GOES-R
                        if (primary_header.file_type_code == 0 && (noaa_header.product_id == 16 ||
                                                                   noaa_header.product_id == 17 ||
                                                                   noaa_header.product_id == 18 ||
                                                                   noaa_header.product_id == 19))
                        {
                            if (goes_r_fc_composer_meso1->hasData)
                                goes_r_fc_composer_meso1->save(directory);
                            if (goes_r_fc_composer_meso2->hasData)
                                goes_r_fc_composer_meso2->save(directory);
                        }
                    }
                }
            }
            // Check if this EMWIN data
            else if (write_emwin && primary_header.file_type_code == 2 && (noaa_header.product_id == 9 || noaa_header.product_id == 6))
            {
                std::string clean_filename = current_filename.substr(0, current_filename.size() - 5); // Remove extensions

                if (noaa_header.noaa_specific_compression == 0) // Uncompressed TXT
                {
                    if (!std::filesystem::exists(directory + "/EMWIN"))
                        std::filesystem::create_directory(directory + "/EMWIN");

                    logger->info("Writing file " + directory + "/EMWIN/" + clean_filename + ".txt" + "...");

                    int offset = primary_header.total_header_length;

                    // Write file out
                    std::ofstream file(directory + "/EMWIN/" + clean_filename + ".txt");
                    file.write((char *)&lrit_data[offset], lrit_data.size() - offset);
                    file.close();
                }
                else if (noaa_header.noaa_specific_compression == 10) // ZIP Files
                {
                    if (!std::filesystem::exists(directory + "/EMWIN"))
                        std::filesystem::create_directory(directory + "/EMWIN");

                    int offset = primary_header.total_header_length;

                    // Init
                    mz_zip_archive zipFile;
                    MZ_CLEAR_OBJ(zipFile);
                    if (!mz_zip_reader_init_mem(&zipFile, &lrit_data[offset], lrit_data.size() - offset, MZ_ZIP_FLAG_DO_NOT_SORT_CENTRAL_DIRECTORY))
                    {
                        logger->error("Could not open ZIP data! Discarding...");
                        return;
                    }

                    // Read filename
                    char filenameStr[1000];
                    int chs = mz_zip_reader_get_filename(&zipFile, 0, filenameStr, 1000);
                    std::string filename(filenameStr, &filenameStr[chs - 1]);

                    // Decompress
                    size_t outSize = 0;
                    uint8_t *outBuffer = (uint8_t *)mz_zip_reader_extract_to_heap(&zipFile, 0, &outSize, 0);

                    // Write out
                    logger->info("Writing file " + directory + "/EMWIN/" + filename + "...");
                    std::ofstream file(directory + "/EMWIN/" + filename);
                    file.write((char *)outBuffer, outSize);
                    file.close();

                    // Free memory
                    delete outBuffer;
                }
            }
            // Check if this is message data. If we slipped to here we know it's not EMWIN
            else if (write_messages && (primary_header.file_type_code == 1 || primary_header.file_type_code == 2))
            {
                std::string clean_filename = current_filename.substr(0, current_filename.size() - 5); // Remove extensions

                if (!std::filesystem::exists(directory + "/Admin Messages"))
                    std::filesystem::create_directory(directory + "/Admin Messages");

                logger->info("Writing file " + directory + "/Admin Messages/" + clean_filename + ".txt" + "...");

                int offset = primary_header.total_header_length;

                // Write file out
                std::ofstream file(directory + "/Admin Messages/" + clean_filename + ".txt");
                file.write((char *)&lrit_data[offset], lrit_data.size() - offset);
                file.close();
            }
            // Check if this is DCS data
            else if (write_dcs && primary_header.file_type_code == 130)
            {
                if (!std::filesystem::exists(directory + "/DCS"))
                    std::filesystem::create_directory(directory + "/DCS");

                logger->info("Writing file " + directory + "/DCS/" + current_filename + "...");

                // Write file out
                std::ofstream file(directory + "/DCS/" + current_filename, std::ios::binary);
                file.write((char *)lrit_data.data(), lrit_data.size());
                file.close();
            }
            // Otherwise, write as generic, unknown stuff. This should not happen
            else if (write_unknown)
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
            if (segmentedDecoder.image_id != -1)
            {
                finalizeLRITData();

                logger->info("Writing image " + directory + "/IMAGES/" + current_filename + ".png" + "...");
                segmentedDecoder.image.save_png(std::string(directory + "/IMAGES/" + current_filename + ".png").c_str());
            }
        }
    } // namespace atms
} // namespace jpss