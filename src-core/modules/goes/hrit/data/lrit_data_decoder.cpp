#include "lrit_data_decoder.h"

#include <iostream>
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
#include "common/miniz/miniz.h"
#include "common/miniz/miniz_zip.h"
#include "common/utils.h"

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
        }

        LRITDataDecoder::~LRITDataDecoder()
        {
        }

        void LRITDataDecoder::work(ccsds::ccsds_1_0_1024::CCSDSPacket &packet)
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

        void LRITDataDecoder::processLRITHeader(ccsds::ccsds_1_0_1024::CCSDSPacket &pkt)
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

                    if (image_structure_record.compression_flag == 1)
                    {
                        is_rice_compressed = true;

                        logger->debug("This is RICE-compressed! Decompressing...");

                        rice_parameters.bits_per_pixel = image_structure_record.bit_per_pixel;
                        rice_parameters.pixels_per_block = 16 * 2; // The real default is 16, but has been to 32 for ages
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

        void LRITDataDecoder::processLRITData(ccsds::ccsds_1_0_1024::CCSDSPacket &pkt)
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

                                std::string subdir = "GOES-" + std::to_string(noaa_header.product_id) + "/" + region;

                                if (!std::filesystem::exists(directory + "/IMAGES/" + subdir))
                                    std::filesystem::create_directories(directory + "/IMAGES/" + subdir);

                                current_filename = subdir + "/" + getHRITImageFilename(timeReadable, "G" + std::to_string(noaa_header.product_id), channel);
                            }
                        }
                    }
                    // Himawari-8 rebroadcast
                    else if (primary_header.file_type_code == 0 && noaa_header.product_id == 43)
                    {
                        std::string subdir = "Himawari-8/Full Disk";

                        if (!std::filesystem::exists(directory + "/IMAGES/" + subdir))
                            std::filesystem::create_directories(directory + "/IMAGES/" + subdir);

                        current_filename = subdir + "/" + getHRITImageFilename(timeReadable, "HIM8", noaa_header.product_subid); // SubID = Channel
                    }
                }

                if (all_headers.count(SegmentIdentificationHeader::TYPE) > 0)
                {
                    SegmentIdentificationHeader segment_id_header(&lrit_data[all_headers[SegmentIdentificationHeader::TYPE]]);

                    if (segmentedDecoder.image_id != segment_id_header.image_identifier)
                    {
                        if (segmentedDecoder.image_id != -1)
                        {
                            logger->info("Writing image " + directory + "/IMAGES/" + current_filename + ".png" + "...");
                            segmentedDecoder.image.save_png(std::string(directory + "/IMAGES/" + current_filename + ".png").c_str());
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

                    if (segmentedDecoder.isComplete())
                    {
                        logger->info("Writing image " + directory + "/IMAGES/" + current_filename + ".png" + "...");
                        segmentedDecoder.image.save_png(std::string(directory + "/IMAGES/" + current_filename + ".png").c_str());
                        segmentedDecoder = SegmentedLRITImageDecoder();
                    }
                }
                else
                {
                    logger->info("Writing image " + directory + "/IMAGES/" + current_filename + ".png" + "...");
                    cimg_library::CImg<unsigned char> image(&lrit_data[primary_header.total_header_length], image_structure_record.columns_count, image_structure_record.lines_count);
                    image.save_png(std::string(directory + "/IMAGES/" + current_filename + ".png").c_str());
                }
            }
            // Check if this EMWIN data
            else if (write_emwin && primary_header.file_type_code == 2 && (noaa_header.product_id == 9 || noaa_header.product_id == 6))
            {
                if (noaa_header.noaa_specific_compression == 0) // Uncompressed TXT
                {
                    if (!std::filesystem::exists(directory + "/EMWIN"))
                        std::filesystem::create_directory(directory + "/EMWIN");

                    logger->info("Writing file " + directory + "/EMWIN/" + current_filename + ".txt" + "...");

                    int offset = primary_header.total_header_length;

                    // Write file out
                    std::ofstream file(directory + "/EMWIN/" + current_filename + ".txt");
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
                if (!std::filesystem::exists(directory + "/Admin Messages"))
                    std::filesystem::create_directory(directory + "/Admin Messages");

                logger->info("Writing file " + directory + "/Admin Messages/" + current_filename + ".txt" + "...");

                int offset = primary_header.total_header_length;

                // Write file out
                std::ofstream file(directory + "/Admin Messages/" + current_filename + ".txt");
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
            }
        }

        SegmentedLRITImageDecoder::SegmentedLRITImageDecoder()
        {
            seg_count = 0;
            seg_height = 0;
            seg_width = 0;
            image_id = -1;
        }

        SegmentedLRITImageDecoder::SegmentedLRITImageDecoder(int max_seg, int segment_width, int segment_height, uint16_t id) : seg_count(max_seg), image_id(id)
        {
            segments_done = std::shared_ptr<bool>(new bool[seg_count], [](bool *p)
                                                  { delete[] p; });
            std::fill(segments_done.get(), &segments_done.get()[seg_count], false);

            image = cimg_library::CImg<unsigned char>(segment_width, segment_height * max_seg, 1);
            seg_height = segment_height;
            seg_width = segment_width;

            image.fill(0);
        }

        SegmentedLRITImageDecoder::~SegmentedLRITImageDecoder()
        {
        }

        void SegmentedLRITImageDecoder::pushSegment(uint8_t *data, int segc)
        {
            if (segc >= seg_count)
                return;
            std::memcpy(&image[(seg_height * seg_width) * segc], data, seg_height * seg_width);
            segments_done.get()[segc] = true;
        }

        bool SegmentedLRITImageDecoder::isComplete()
        {
            bool complete = true;
            for (int i = 0; i < seg_count; i++)
                complete = complete && segments_done.get()[i];
            return complete;
        }
    } // namespace atms
} // namespace jpss