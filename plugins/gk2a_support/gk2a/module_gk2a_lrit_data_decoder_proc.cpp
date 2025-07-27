#include "image/io.h"
#include "image/jpeg12_utils.h"
#include "image/jpeg_utils.h"
#include "imgui/imgui_image.h"
#include "logger.h"
#include "lrit_header.h"
#include "module_gk2a_lrit_data_decoder.h"
#include "utils/string.h"
#include "xrit/gk2a/decomp.h"
#include "xrit/gk2a/gk2a_headers.h"
#include <filesystem>
#include <fstream>

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

            if (file.custom_flags[satdump::xrit::gk2a::IS_ENCRYPTED] && decryption_keys.size() > 0) // Decrypt
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

                uint64_t key = decryption_keys[file.custom_flags[satdump::xrit::gk2a::KEY_INDEX]];

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

            // CHeck if we lack decryption, if so, we're stuck
            if (file.custom_flags[satdump::xrit::gk2a::IS_ENCRYPTED] && decryption_keys.size() <= 0)
            {
                if (!std::filesystem::exists(directory + "/LRIT_ENCRYPTED"))
                    std::filesystem::create_directory(directory + "/LRIT_ENCRYPTED");

                logger->info("Writing file " + directory + "/LRIT_ENCRYPTED/" + file.filename + "...");

                // Write file out
                std::ofstream fileo(directory + "/LRIT_ENCRYPTED/" + file.filename, std::ios::binary);
                fileo.write((char *)file.lrit_data.data(), file.lrit_data.size());
                fileo.close();
                return;
            }

            satdump::xrit::XRITFileInfo finfo = satdump::xrit::identifyXRITFIle(file);

            // Check if this is image data, and if so also write it as an image
            if (finfo.type != satdump::xrit::XRIT_UNKNOWN)
            {
#if 1
                if (primary_header.file_type_code == 0 && file.hasHeader<::lrit::ImageStructureRecord>())
                    if (finfo.type == satdump::xrit::XRIT_GK2A_AMI)
                        satdump::xrit::gk2a::decompressGK2AHritFileIfRequired(file);

                processor.push(finfo, file);
#else
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

                    GK2AxRITProductMeta lmeta;
                    lmeta.filename = file.filename;

                    ::lrit::ImageStructureRecord image_structure_record = file.getHeader<::lrit::ImageStructureRecord>();

                    if (file.custom_flags[JPG_COMPRESSED] || file.custom_flags[J2K_COMPRESSED]) // Is this Jpeg-Compressed? Decompress
                    {
                        logger->info("Decompressing JPEG...");
                        image::Image img;
                        if (image_structure_record.bit_per_pixel > 8)
                        {
                            img = image::decompress_jpeg12(&file.lrit_data[primary_header.total_header_length], file.lrit_data.size() - primary_header.total_header_length);
                            for (size_t c = 0; c < img.size(); c++)
                                img.set(c, img.get(c) << 2);
                        }
                        else
                            img = image::decompress_jpeg(&file.lrit_data[primary_header.total_header_length], file.lrit_data.size() - primary_header.total_header_length);

                        if (img.width() < image_structure_record.columns_count || img.height() < image_structure_record.lines_count)
                            img.init(image_structure_record.bit_per_pixel > 8 ? 16 : 8, image_structure_record.columns_count, image_structure_record.lines_count, 1); // Just in case it's corrupted!

                        file.lrit_data.erase(file.lrit_data.begin() + primary_header.total_header_length, file.lrit_data.end());
                        file.lrit_data.insert(file.lrit_data.end(), (uint8_t *)img.raw_data(), (uint8_t *)img.raw_data() + img.size() * img.typesize());
                    }

                    std::vector<std::string> header_parts = satdump::splitString(current_filename, '_'); // Is this a FD?
                    if (header_parts.size() < 2)
                        header_parts = {"", ""};

                    // Only FDs are supposed to come down. We only handle other headers in this situation.
                    if (header_parts.size() >= 6 && header_parts[1] == "FD")
                    {
                        lmeta.channel = header_parts[3];
                        lmeta.satellite_name = "GK-2A";
                        lmeta.satellite_short_name = "GK2A";
                        // lmeta.scan_time = time(0);
                        {
                            time_t tttt = time(0);
                            std::tm scanTimestamp = *gmtime(&tttt);
                            std::string scanTime = header_parts[4] + header_parts[5];
                            strptime(scanTime.c_str(), "%Y%m%d%H%M%S", &scanTimestamp);
                            lmeta.scan_time = timegm(&scanTimestamp);
                        }

                        // Try to parse navigation
                        if (file.hasHeader<::lrit::ImageNavigationRecord>())
                        {
                            lmeta.image_navigation_record = std::make_shared<::lrit::ImageNavigationRecord>(file.getHeader<::lrit::ImageNavigationRecord>());
                            lmeta.image_navigation_record->line_scaling_factor = -lmeta.image_navigation_record->line_scaling_factor; // Little GK-2A Quirk...
                        }

                        // Try to parse calibration
                        if (file.hasHeader<::lrit::ImageDataFunctionRecord>())
                            lmeta.image_data_function_record = std::make_shared<::lrit::ImageDataFunctionRecord>(file.getHeader<::lrit::ImageDataFunctionRecord>());

                        if (all_wip_images.count(lmeta.channel) == 0)
                            all_wip_images.insert({lmeta.channel, std::make_unique<wip_images>()});

                        std::unique_ptr<wip_images> &wip_img = all_wip_images[lmeta.channel];

                        if (segmentedDecoders.count(lmeta.channel) <= 0)
                        {
                            segmentedDecoders.insert(std::pair<std::string, SegmentedLRITImageDecoder>(lmeta.channel, SegmentedLRITImageDecoder()));
                            wip_img->imageStatus = IDLE;
                            wip_img->img_height = 0;
                            wip_img->img_width = 0;
                        }

                        wip_img->imageStatus = RECEIVING;

                        std::string orig_filename = current_filename;
                        std::string image_id = current_filename.substr(0, current_filename.size() - 8);

                        SegmentedLRITImageDecoder &segmentedDecoder = segmentedDecoders[lmeta.channel];

                        if (segmentedDecoder.image_id != image_id)
                        {
                            if (segmentedDecoder.image_id != "")
                            {
                                current_filename = image_id;

                                wip_img->imageStatus = SAVING;
                                saveImageP(segmentedDecoder.meta, segmentedDecoder.image);
                                wip_img->imageStatus = RECEIVING;
                            }

                            segmentedDecoder =
                                SegmentedLRITImageDecoder(image_structure_record.bit_per_pixel > 8 ? 16 : 8, 10, image_structure_record.columns_count, image_structure_record.lines_count, image_id);
                            segmentedDecoder.meta = lmeta;
                        }

                        std::vector<std::string> header_parts = satdump::splitString(orig_filename, '_');

                        int seg_number = 0;
                        if (header_parts.size() >= 7)
                            seg_number = std::stoi(header_parts[6].substr(0, header_parts.size() - 4)) - 1;
                        else
                            logger->critical("Could not parse segment number from filename!");
                        image::Image image(&file.lrit_data[primary_header.total_header_length], image_structure_record.bit_per_pixel > 8 ? 16 : 8, image_structure_record.columns_count,
                                           image_structure_record.lines_count, 1);
                        segmentedDecoder.pushSegment(image, seg_number);

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
                            saveImageP(segmentedDecoder.meta, segmentedDecoder.image);
                            segmentedDecoder = SegmentedLRITImageDecoder();
                            wip_img->imageStatus = IDLE;
                        }
                    }
                    else
                    {
                        lmeta.filename = current_filename.substr(0, current_filename.size() - 5); // Remove extensions
                        // Write raw image dats
                        image::Image image(&file.lrit_data[primary_header.total_header_length], 8, image_structure_record.columns_count, image_structure_record.lines_count, 1);
                        saveImageP(lmeta, image);
                    }
                }
#endif
            }
            else if (primary_header.file_type_code == 255)
            {
                std::string clean_filename = current_filename.substr(0, current_filename.size() - 5); // Remove extensions

                int offset = primary_header.total_header_length;

                std::vector<std::string> header_parts = satdump::splitString(current_filename, '_');

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
    } // namespace lrit
} // namespace gk2a