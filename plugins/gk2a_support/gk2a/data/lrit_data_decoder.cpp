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
#include "libs/miniz/miniz.h"
#include "libs/miniz/miniz_zip.h"
#include "common/utils.h"
#include "libs/others/strptime.h"
#include "imgui/imgui_image.h"
#include "common/image/jpeg_utils.h"
#include "resources.h"
extern "C"
{
#include "libs/libtom/tomcrypt_des.c"
}

namespace gk2a
{
    namespace lrit
    {
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

            if (resources::resourceExists("gk2a/EncryptionKeyMessage.bin"))
            {
                // Load decryption keys
                std::ifstream keyFile(resources::getResourcePath("gk2a/EncryptionKeyMessage.bin"), std::ios::binary);

                // Read key count
                uint16_t key_count = 0;
                keyFile.read((char *)&key_count, 2);
                key_count = (key_count & 0xFF) << 8 | (key_count >> 8);

                uint8_t readBuf[10];
                for (int i = 0; i < key_count; i++)
                {
                    keyFile.read((char *)readBuf, 10);
                    int index = readBuf[0] << 8 | readBuf[1];
                    uint64_t key = (uint64_t)readBuf[2] << 56 |
                                   (uint64_t)readBuf[3] << 48 |
                                   (uint64_t)readBuf[4] << 40 |
                                   (uint64_t)readBuf[5] << 32 |
                                   (uint64_t)readBuf[6] << 24 |
                                   (uint64_t)readBuf[7] << 16 |
                                   (uint64_t)readBuf[8] << 8 |
                                   (uint64_t)readBuf[9];
                    std::memcpy(&key, &readBuf[2], 8);
                    decryption_keys.emplace(std::pair<int, uint64_t>(index, key));
                }

                keyFile.close();
            }
            else
            {
                logger->critical("GK-2A Decryption keys could not be loaded!!! Nothing apart from encrypted data will be decoded.");
            }
        }

        LRITDataDecoder::~LRITDataDecoder()
        {
        }

        void LRITDataDecoder::work(ccsds::CCSDSPacket &packet)
        {
            // Thanks Sam for this one... Defeats the very purpose of those marker but that's an eof CCSDS frame. We'll just ignore those.
            if (packet.header.packet_sequence_count == 0 && packet.header.apid == 0 && packet.payload.size() == 0 && packet.header.sequence_flag == 0)
                return;

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
                    }
                    else if (image_structure_record.compression_flag == 1 /* JPEG 2000 */)
                    {
                        logger->debug("JPEG2000 Compression is used, decompressing...");
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
                    if (key_header.key != 0)
                    {
                        logger->debug("This is encrypted!");
                        is_encrypted = true;
                        key_index = key_header.key;
                    }
                    else
                    {
                        is_encrypted = false;
                    }
                }
                else
                {
                    is_encrypted = false;
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

            if (is_encrypted && decryption_keys.size() > 0) // Decrypt
            {
                logger->info("Decrypting....");
                uint8_t inblock[8], outblock[8];

                int payloadSize = (lrit_data.size() - primary_header.total_header_length);

                // Do we need to pad the last block?
                int pad = payloadSize % 8;
                if (pad != 0)
                {
                    for (int i = 0; i < pad; i++)
                        lrit_data.push_back(0x00);
                }

                int blocks = (payloadSize + pad) / 8;

                uint64_t key = decryption_keys[key_index];

                // Init a libtom instance
                symmetric_key sk;
                des_setup((unsigned char *)&key, 8, 0, &sk);

                uint8_t *encrypted_ptr = &lrit_data[primary_header.total_header_length];

                // Decrypt
                for (int b = 0; b < blocks; b++)
                {
                    std::memcpy(inblock, &encrypted_ptr[b * 8], 8);

                    // Has to be ran twice to do the entire thing
                    des_ecb_decrypt(inblock, outblock, &sk);
                    des_ecb_decrypt(inblock, outblock, &sk);

                    std::memcpy(&encrypted_ptr[b * 8], outblock, 8);
                }

                // Erase compression header
                lrit_data[all_headers[KeyHeader::TYPE]] = 0; // Type
            }

            // Check if this is image data, and if so also write it as an image
            if (primary_header.file_type_code == 0 && all_headers.count(ImageStructureRecord::TYPE) > 0)
            {
                if (is_encrypted && decryption_keys.size() <= 0) // We lack decryption
                {
                    if (!std::filesystem::exists(directory + "/LRIT_ENCRYPTED"))
                        std::filesystem::create_directory(directory + "/LRIT_ENCRYPTED");

                    logger->info("Writing file " + directory + "/LRIT_ENCRYPTED/" + current_filename + "...");

                    // Write file out
                    std::ofstream file(directory + "/LRIT_ENCRYPTED/" + current_filename, std::ios::binary);
                    file.write((char *)lrit_data.data(), lrit_data.size());
                    file.close();
                }
                else
                {
                    if (!std::filesystem::exists(directory + "/IMAGES"))
                        std::filesystem::create_directory(directory + "/IMAGES");

                    ImageStructureRecord image_structure_record(&lrit_data[all_headers[ImageStructureRecord::TYPE]]);

                    if (is_jpeg_compressed) // Is this Jpeg-Compressed? Decompress
                    {
                        logger->info("Decompressing JPEG...");
                        image::Image<uint8_t> img = image::decompress_jpeg(&lrit_data[primary_header.total_header_length], lrit_data.size() - primary_header.total_header_length);
                        lrit_data.erase(lrit_data.begin() + primary_header.total_header_length, lrit_data.end());
                        lrit_data.insert(lrit_data.end(), (uint8_t *)&img[0], (uint8_t *)&img[img.height() * img.width()]);
                    }

                    std::vector<std::string> header_parts = splitString(current_filename, '_'); // Is this a FD?
                    if (header_parts.size() < 2)
                        header_parts = {"", ""};

                    if (header_parts[1] == "FD")
                    {
                        std::string channel = header_parts[3];

                        if (segmentedDecoders.count(channel) <= 0)
                        {
                            segmentedDecoders.insert(std::pair<std::string, SegmentedLRITImageDecoder>(channel, SegmentedLRITImageDecoder()));
                            imageStatus[channel] = IDLE;
                            img_heights[channel] = 0;
                            img_widths[channel] = 0;
                        }

                        imageStatus[channel] = RECEIVING;

                        std::string image_id = current_filename.substr(0, current_filename.size() - 8);

                        SegmentedLRITImageDecoder &segmentedDecoder = segmentedDecoders[channel];

                        if (segmentedDecoder.image_id != image_id)
                        {
                            if (segmentedDecoder.image_id != "")
                            {
                                current_filename = image_id;

                                imageStatus[channel] = SAVING;
                                logger->info("Writing image " + directory + "/IMAGES/" + current_filename + ".png" + "...");
                                segmentedDecoder.image.save_png(std::string(directory + "/IMAGES/" + current_filename + ".png").c_str());
                                imageStatus[channel] = RECEIVING;
                            }

                            segmentedDecoder = SegmentedLRITImageDecoder(10,
                                                                         image_structure_record.columns_count,
                                                                         image_structure_record.lines_count,
                                                                         image_id);
                        }

                        std::vector<std::string> header_parts = splitString(current_filename, '_');

                        int seg_number = 0;
                        if (header_parts.size() >= 7)
                        {
                            seg_number = std::stoi(header_parts[6].substr(0, header_parts.size() - 4)) - 1;
                        }
                        else
                        {
                            logger->critical("Could not parse segment number from filename!");
                        }
                        segmentedDecoder.pushSegment(&lrit_data[primary_header.total_header_length], seg_number);

                        // If the UI is active, update texture
                        if (textureIDs[channel] > 0)
                        {
                            // Downscale image
                            img_heights[channel] = 1000;
                            img_widths[channel] = 1000;
                            image::Image<uint8_t> imageScaled = segmentedDecoder.image;
                            imageScaled.resize(img_widths[channel], img_heights[channel]);
                            uchar_to_rgba(imageScaled.data(), textureBuffers[channel], img_heights[channel] * img_widths[channel]);
                            hasToUpdates[channel] = true;
                        }

                        if (segmentedDecoder.isComplete())
                        {
                            current_filename = image_id;

                            imageStatus[channel] = SAVING;
                            logger->info("Writing image " + directory + "/IMAGES/" + current_filename + ".png" + "...");
                            segmentedDecoder.image.save_png(std::string(directory + "/IMAGES/" + current_filename + ".png").c_str());
                            segmentedDecoder = SegmentedLRITImageDecoder();
                            imageStatus[channel] = IDLE;
                        }
                    }
                    else
                    {
                        std::string clean_filename = current_filename.substr(0, current_filename.size() - 5); // Remove extensions
                        // Write raw image dats
                        logger->info("Writing image " + directory + "/IMAGES/" + clean_filename + ".png" + "...");
                        image::Image<uint8_t> image(&lrit_data[primary_header.total_header_length], image_structure_record.columns_count, image_structure_record.lines_count, 1);
                        image.save_png(std::string(directory + "/IMAGES/" + clean_filename + ".png").c_str());
                    }
                }
            }
            else if (primary_header.file_type_code == 255)
            {
                std::string clean_filename = current_filename.substr(0, current_filename.size() - 5); // Remove extensions

                int offset = primary_header.total_header_length;

                std::vector<std::string> header_parts = splitString(current_filename, '_');

                // Get type name
                std::string name = "";
                if (header_parts.size() >= 2)
                    name = header_parts[1] + "/";

                // Get extension
                std::string extension = "bin";
                if (std::string((char *)&lrit_data[offset + 1], 3) == "PNG")
                    extension = "png";
                else if (std::string((char *)&lrit_data[offset], 3) == "GIF")
                    extension = "gif";
                else if (name == "ANT")
                    extension = "txt";

                if (!std::filesystem::exists(directory + "/ADD/" + name))
                    std::filesystem::create_directories(directory + "/ADD/" + name);

                logger->info("Writing file " + directory + "/ADD/" + name + clean_filename + "." + extension + "...");

                // Write file out
                std::ofstream file(directory + "/ADD/" + name + clean_filename + "." + extension);
                file.write((char *)&lrit_data[offset], lrit_data.size() - offset);
                file.close();
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
            if (segmentedDecoders.size() > 0)
            {
                for (std::pair<const std::string, SegmentedLRITImageDecoder> &dec : segmentedDecoders)
                {
                    SegmentedLRITImageDecoder &segmentedDecoder = dec.second;

                    if (segmentedDecoder.image_id != "")
                    {
                        finalizeLRITData();

                        logger->info("Writing image " + directory + "/IMAGES/" + current_filename + ".png" + "...");
                        segmentedDecoder.image.save_png(std::string(directory + "/IMAGES/" + current_filename + ".png").c_str());
                    }
                }
            }
        }
    } // namespace atms
} // namespace jpss