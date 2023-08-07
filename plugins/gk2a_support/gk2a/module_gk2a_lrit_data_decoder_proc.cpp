#include "module_gk2a_lrit_data_decoder.h"
#include "logger.h"
#include "lrit_header.h"
#include <fstream>
#include "common/image/jpeg_utils.h"
#include "imgui/imgui_image.h"

extern "C"
{
#include "libtom/tomcrypt_des.c"
}

namespace gk2a
{
    namespace lrit
    {
        void GK2ALRITDataDecoderModule::processLRITFile(::lrit::LRITFile &file)
        {
            std::string current_filename = file.filename;

            ::lrit::PrimaryHeader primary_header = file.getHeader<::lrit::PrimaryHeader>();

            if (file.custom_flags[IS_ENCRYPTED] && decryption_keys.size() > 0) // Decrypt
            {
                logger->info("Decrypting....");
                uint8_t inblock[8], outblock[8];

                int payloadSize = (file.lrit_data.size() - primary_header.total_header_length);

                // Do we need to pad the last block?
                int pad = payloadSize % 8;
                if (pad != 0)
                {
                    for (int i = 0; i < pad; i++)
                        file.lrit_data.push_back(0x00);
                }

                int blocks = (payloadSize + pad) / 8;

                uint64_t key = decryption_keys[file.custom_flags[KEY_INDEX]];

                // Init a libtom instance
                symmetric_key sk;
                des_setup((unsigned char *)&key, 8, 0, &sk);

                uint8_t *encrypted_ptr = &file.lrit_data[primary_header.total_header_length];

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
                file.lrit_data[file.all_headers[KeyHeader::TYPE]] = 0; // Type
            }

            // Check if this is image data, and if so also write it as an image
            if (primary_header.file_type_code == 0 && file.hasHeader<::lrit::ImageStructureRecord>())
            {
                if (file.custom_flags[IS_ENCRYPTED] && decryption_keys.size() <= 0) // We lack decryption
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
                    if (!std::filesystem::exists(directory + "/IMAGES"))
                        std::filesystem::create_directory(directory + "/IMAGES");

                    ::lrit::ImageStructureRecord image_structure_record = file.getHeader<::lrit::ImageStructureRecord>();

                    if (file.custom_flags[JPG_COMPRESSED] || file.custom_flags[J2K_COMPRESSED]) // Is this Jpeg-Compressed? Decompress
                    {
                        logger->info("Decompressing JPEG...");
                        image::Image<uint8_t> img = image::decompress_jpeg(&file.lrit_data[primary_header.total_header_length], file.lrit_data.size() - primary_header.total_header_length);
                        if (img.width() < image_structure_record.columns_count || img.height() < image_structure_record.lines_count)
                            img.init(image_structure_record.columns_count, image_structure_record.lines_count, 1); // Just in case it's corrupted!
                        file.lrit_data.erase(file.lrit_data.begin() + primary_header.total_header_length, file.lrit_data.end());
                        file.lrit_data.insert(file.lrit_data.end(), (uint8_t *)&img[0], (uint8_t *)&img[img.height() * img.width()]);
                    }

                    std::vector<std::string> header_parts = splitString(current_filename, '_'); // Is this a FD?
                    if (header_parts.size() < 2)
                        header_parts = {"", ""};

                    if (header_parts[1] == "FD")
                    {
                        std::string channel = header_parts[3];

                        if (all_wip_images.count(channel) == 0)
                            all_wip_images.insert({channel, std::make_unique<wip_images>()});

                        std::unique_ptr<wip_images> &wip_img = all_wip_images[channel];

                        if (segmentedDecoders.count(channel) <= 0)
                        {
                            segmentedDecoders.insert(std::pair<std::string, SegmentedLRITImageDecoder>(channel, SegmentedLRITImageDecoder()));
                            wip_img->imageStatus = IDLE;
                            wip_img->img_height = 0;
                            wip_img->img_width = 0;
                        }

                        wip_img->imageStatus = RECEIVING;

                        std::string orig_filename = current_filename;
                        std::string image_id = current_filename.substr(0, current_filename.size() - 8);

                        SegmentedLRITImageDecoder &segmentedDecoder = segmentedDecoders[channel];

                        if (segmentedDecoder.image_id != image_id)
                        {
                            if (segmentedDecoder.image_id != "")
                            {
                                current_filename = image_id;

                                wip_img->imageStatus = SAVING;
                                segmentedDecoder.image.save_img(std::string(directory + "/IMAGES/" + current_filename).c_str());
                                wip_img->imageStatus = RECEIVING;
                            }

                            segmentedDecoder = SegmentedLRITImageDecoder(10,
                                                                         image_structure_record.columns_count,
                                                                         image_structure_record.lines_count,
                                                                         image_id);
                        }

                        std::vector<std::string> header_parts = splitString(orig_filename, '_');

                        int seg_number = 0;
                        if (header_parts.size() >= 7)
                            seg_number = std::stoi(header_parts[6].substr(0, header_parts.size() - 4)) - 1;
                        else
                            logger->critical("Could not parse segment number from filename!");
                        segmentedDecoder.pushSegment(&file.lrit_data[primary_header.total_header_length], seg_number);

                        // If the UI is active, update texture
                        if (wip_img->textureID > 0)
                        {
                            // Downscale image
                            wip_img->img_height = 1000;
                            wip_img->img_width = 1000;
                            image::Image<uint8_t> imageScaled = segmentedDecoder.image;
                            imageScaled.resize(wip_img->img_width, wip_img->img_height);
                            uchar_to_rgba(imageScaled.data(), wip_img->textureBuffer, wip_img->img_height * wip_img->img_width);
                            wip_img->hasToUpdate = true;
                        }

                        if (segmentedDecoder.isComplete())
                        {
                            current_filename = image_id;

                            wip_img->imageStatus = SAVING;
                            segmentedDecoder.image.save_img(std::string(directory + "/IMAGES/" + current_filename).c_str());
                            segmentedDecoder = SegmentedLRITImageDecoder();
                            wip_img->imageStatus = IDLE;
                        }
                    }
                    else
                    {
                        std::string clean_filename = current_filename.substr(0, current_filename.size() - 5); // Remove extensions
                        // Write raw image dats
                        image::Image<uint8_t> image(&file.lrit_data[primary_header.total_header_length], image_structure_record.columns_count, image_structure_record.lines_count, 1);
                        image.save_img(std::string(directory + "/IMAGES/" + clean_filename).c_str());
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
                if (std::string((char *)&file.lrit_data[offset + 1], 3) == "PNG")
                    extension = "png";
                else if (std::string((char *)&file.lrit_data[offset], 3) == "GIF")
                    extension = "gif";
                else if (name == "ANT")
                    extension = "txt";

                if (!std::filesystem::exists(directory + "/ADD/" + name))
                    std::filesystem::create_directories(directory + "/ADD/" + name);

                logger->info("Writing file " + directory + "/ADD/" + name + clean_filename + "." + extension + "...");

                // Write file out
                std::ofstream fileo(directory + "/ADD/" + name + clean_filename + "." + extension, std::ios::binary);
                fileo.write((char *)&file.lrit_data[offset], file.lrit_data.size() - offset);
                fileo.close();
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
    } // namespace avhrr
} // namespace metop