#include "DecompWT/CompressT4.h"
#include "DecompWT/CompressWT.h"
#include "image/io.h"
#include "image/jpeg_utils.h"
#include "imgui/imgui_image.h"
#include "logger.h"
#include "lrit_header.h"
#include "module_elektro_lrit_data_decoder.h"
#include "utils/string.h"
#include "xrit/identify.h"
#include "xrit/msg/decomp.h"
#include <filesystem>

namespace elektro
{
    namespace lrit
    {
        void ELEKTROLRITDataDecoderModule::processLRITFile(::lrit::LRITFile &file)
        {
            std::string current_filename = file.filename;

            ::lrit::PrimaryHeader primary_header = file.getHeader<::lrit::PrimaryHeader>();

            satdump::xrit::XRITFileInfo finfo = satdump::xrit::identifyXRITFIle(file);

            // Check if this is image data, and if so also write it as an image
            if (finfo.type != satdump::xrit::XRIT_UNKNOWN)
            {
#if 1
                if (primary_header.file_type_code == 0 && file.hasHeader<::lrit::ImageStructureRecord>())
                    if (finfo.type == satdump::xrit::XRIT_ELEKTRO_MSUGS || finfo.type == satdump::xrit::XRIT_MSG_SEVIRI)
                        satdump::xrit::msg::decompressMsgHritFileIfRequired(file);

                processor.push(finfo, file);
#else
                if (!std::filesystem::exists(directory + "/IMAGES"))
                    std::filesystem::create_directory(directory + "/IMAGES");

                ::lrit::ImageStructureRecord image_structure_record = file.getHeader<::lrit::ImageStructureRecord>();

                if (file.custom_flags[JPEG_COMPRESSED]) // Is this Jpeg-Compressed? Decompress
                {
                    logger->info("Decompressing JPEG...");
                    image::Image img = image::decompress_jpeg(&file.lrit_data[primary_header.total_header_length], file.lrit_data.size() - primary_header.total_header_length, true);
                    if (img.depth() != 8)
                        logger->error("ELEKTRO xRIT JPEG Depth should be 8!");

                    file.lrit_data.erase(file.lrit_data.begin() + primary_header.total_header_length, file.lrit_data.end());
                    file.lrit_data.insert(file.lrit_data.end(), (uint8_t *)img.raw_data(), (uint8_t *)img.raw_data() + img.height() * img.width());
                }
                else if (file.custom_flags[WT_COMPRESSED]) // Is this Wavelet-Compressed? Decompress. We know this will always be 10-bits
                {
                    /*
                    This could be better... Sometimes I should maybe adapt the DecompWT code to be cleaner for this purpose...
                    */
                    logger->info("Decompressing Wavelet...");

                    int compression_type = 3; // We assume Wavelet

                    if (file.all_headers.count(SegmentIdentificationHeader::TYPE) > 0) // But if we have a header with better info, use it
                    {
                        SegmentIdentificationHeader segment_id_header = file.getHeader<SegmentIdentificationHeader>();
                        compression_type = segment_id_header.compression;
                    }

                    // Init variables
                    uint8_t *image_ptr = NULL;
                    int buf_size = 0;
                    int out_pixels = 0;

                    // Decompress
                    {
                        // We need to copy over that memory to its own buffer
                        uint8_t *compressedData = new uint8_t[file.lrit_data.size() - primary_header.total_header_length];
                        std::memcpy(compressedData, &file.lrit_data[primary_header.total_header_length], file.lrit_data.size() - primary_header.total_header_length);

                        // Images object
                        Util::CDataFieldCompressedImage compressedImage(compressedData, (file.lrit_data.size() - primary_header.total_header_length) * 8, image_structure_record.bit_per_pixel,
                                                                        image_structure_record.columns_count, image_structure_record.lines_count);
                        Util::CDataFieldUncompressedImage decompressedImage;

                        // Perform WT Decompression
                        try
                        {
                            std::vector<short> m_QualityInfo;
                            if (compression_type == 3)
                                COMP::DecompressWT(compressedImage, image_structure_record.bit_per_pixel, decompressedImage, m_QualityInfo); // Standard HRIT compression
                            else if (compression_type == 2)
                                COMP::DecompressT4(compressedImage, decompressedImage, m_QualityInfo); // Shouldn't happen but better be ready
                            else
                                logger->error("Unknown compression! This should not have happened!");
                        }
                        catch (std::exception &e)
                        {
                            logger->error("Failed decompression! %d", e.what());
                        }

                        // Fill our output buffer
                        buf_size = decompressedImage.Size() / 8;
                        out_pixels = decompressedImage.Size() / image_structure_record.bit_per_pixel;
                        image_ptr = new uint8_t[buf_size];
                        decompressedImage.Read(0, image_ptr, buf_size);
                    }

                    if (out_pixels == image_structure_record.columns_count * image_structure_record.lines_count) // Matches?
                    {
                        // Empty current LRIT file
                        file.lrit_data.erase(file.lrit_data.begin() + primary_header.total_header_length, file.lrit_data.end());

                        if (image_structure_record.bit_per_pixel == 10)
                        {
                            // Fill it back up converting to 16-bits
                            for (int i = 0; i < buf_size - (buf_size % 5); i += 5)
                            {
                                uint16_t v1 = (image_ptr[0] << 2) | (image_ptr[1] >> 6);
                                uint16_t v2 = ((image_ptr[1] % 64) << 4) | (image_ptr[2] >> 4);
                                uint16_t v3 = ((image_ptr[2] % 16) << 6) | (image_ptr[3] >> 2);
                                uint16_t v4 = ((image_ptr[3] % 4) << 8) | image_ptr[4];

                                v1 <<= 6;
                                v2 <<= 6;
                                v3 <<= 6;
                                v4 <<= 6;

                                file.lrit_data.push_back(v1 & 0xFF);
                                file.lrit_data.push_back(v1 >> 8);
                                file.lrit_data.push_back(v2 & 0xFF);
                                file.lrit_data.push_back(v2 >> 8);
                                file.lrit_data.push_back(v3 & 0xFF);
                                file.lrit_data.push_back(v3 >> 8);
                                file.lrit_data.push_back(v4 & 0xFF);
                                file.lrit_data.push_back(v4 >> 8);

                                image_ptr += 5;
                            }

                            // Fill remaining if required
                            for (int i = 0; i < buf_size % 5; i++)
                                file.lrit_data.push_back(0);
                        }
                        else if (image_structure_record.bit_per_pixel == 8) // Just in case
                        {
                            file.lrit_data.insert(file.lrit_data.end(), image_ptr, &image_ptr[buf_size]);
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
                        file.lrit_data.erase(file.lrit_data.begin() + primary_header.total_header_length, file.lrit_data.end());

                        // Fill with 0s
                        for (int i = 0; i < out_pixels; i++)
                            file.lrit_data.push_back(0);
                    }

                    // Free up memory if necessary
                    // if (image_ptr != NULL)
                    //    delete[] image_ptr;
                }

                if (file.all_headers.count(SegmentIdentificationHeader::TYPE) > 0)
                {
                    SegmentIdentificationHeader segment_id_header = file.getHeader<SegmentIdentificationHeader>();

                    std::vector<std::string> header_parts = satdump::splitString(current_filename, '_');

                    // for (std::string part : header_parts)
                    //     logger->trace(part);

                    std::string image_id = current_filename.substr(0, 30);

                    GOMSxRITProductMeta lmeta;
                    lmeta.filename = file.filename;

                    // Channel / Bit depth
                    lmeta.channel = segment_id_header.channel_id + 1;
                    lmeta.bit_depth = image_structure_record.bit_per_pixel;

                    // Try to parse navigation
                    if (file.hasHeader<::lrit::ImageNavigationRecord>())
                        lmeta.image_navigation_record = std::make_shared<::lrit::ImageNavigationRecord>(file.getHeader<::lrit::ImageNavigationRecord>());

                    // Timestamp
                    std::string timestamp = current_filename.substr(46, 12);
                    std::tm scanTimestamp;
                    strptime(timestamp.c_str(), "%Y%m%d%H%M", &scanTimestamp);
                    scanTimestamp.tm_sec = 0; // No seconds
                    lmeta.scan_time = timegm(&scanTimestamp);

                    // If we can, use a better filename
                    {
                        std::string sat_name = current_filename.substr(6, 5);

                        if (sat_name == "GOMS1") // Dead... But good measure
                        {
                            lmeta.satellite_name = "ELEKTRO-L1";
                            lmeta.satellite_short_name = "L1";
                        }
                        else if (sat_name == "GOMS2")
                        {
                            lmeta.satellite_name = "ELEKTRO-L2";
                            lmeta.satellite_short_name = "L2";
                        }
                        else if (sat_name == "GOMS3")
                        {
                            lmeta.satellite_name = "ELEKTRO-L3";
                            lmeta.satellite_short_name = "L3";
                        }
                        else if (sat_name == "GOMS4")
                        {
                            lmeta.satellite_name = "ELEKTRO-L4";
                            lmeta.satellite_short_name = "L4";
                        }
                        else if (sat_name == "GOMS5") // Not launched yet, but we can expect it to be the same anyway
                        {
                            lmeta.satellite_name = "ELEKTRO-L5";
                            lmeta.satellite_short_name = "L5";
                        }
                    }

                    if (all_wip_images.count(lmeta.channel) == 0)
                        all_wip_images.insert({lmeta.channel, std::make_unique<wip_images>()});

                    std::unique_ptr<wip_images> &wip_img = all_wip_images[lmeta.channel];

                    if (segmentedDecoders.count(lmeta.channel) == 0)
                        segmentedDecoders.insert({lmeta.channel, SegmentedLRITImageDecoder()});

                    SegmentedLRITImageDecoder &segmentedDecoder = segmentedDecoders[lmeta.channel];

                    if (lmeta.image_navigation_record)
                        lmeta.image_navigation_record->line_offset = lmeta.image_navigation_record->line_offset + (segment_id_header.segment_sequence_number - 1) * image_structure_record.lines_count;

                    if (segmentedDecoder.image_id != image_id)
                    {
                        if (segmentedDecoder.image_id != "")
                        {
                            wip_img->imageStatus = SAVING;
                            saveImageP(segmentedDecoder.meta, segmentedDecoder.image);
                            wip_img->imageStatus = RECEIVING;
                        }

                        segmentedDecoder = SegmentedLRITImageDecoder(image_structure_record.bit_per_pixel > 8 ? 16 : 8, segment_id_header.planned_end_segment, image_structure_record.columns_count,
                                                                     image_structure_record.lines_count, image_id);
                        segmentedDecoder.meta = lmeta;
                    }

                    int seg_number = segment_id_header.segment_sequence_number - 1;
                    {
                        image::Image image(&file.lrit_data[primary_header.total_header_length], image_structure_record.bit_per_pixel > 8 ? 16 : 8, image_structure_record.columns_count,
                                           image_structure_record.lines_count, 1);
                        segmentedDecoder.pushSegment(image, seg_number);
                    }

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
                { // Left just in case, should not happen on ELEKTRO-L
                    // Write raw image dats
                    image::Image image(&file.lrit_data[primary_header.total_header_length], image_structure_record.bit_per_pixel > 8 ? 16 : 8, image_structure_record.columns_count,
                                       image_structure_record.lines_count, 1);
                    image::save_img(image, std::string(directory + "/IMAGES/Unknown/" + current_filename).c_str());
                }
#endif // TODOREWORK GUI IMAGE!!!!!
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
} // namespace elektro