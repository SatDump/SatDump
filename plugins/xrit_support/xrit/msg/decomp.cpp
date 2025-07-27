#include "decomp.h"
#include "DecompWT/CompressT4.h"
#include "DecompWT/CompressWT.h"
#include "image/jpeg_utils.h"
#include "logger.h"

namespace satdump
{
    namespace xrit
    {
        namespace msg
        {
            void decompressMsgHritFileIfRequired(::lrit::LRITFile &file)
            {
                ::lrit::PrimaryHeader primary_header = file.getHeader<::lrit::PrimaryHeader>();

                // Only image data can be compressed
                if (primary_header.file_type_code != 0 || !file.hasHeader<::lrit::ImageStructureRecord>())
                {
                    logger->error("File " + file.filename + " is not an image!");
                    return;
                }

                // Get image structure record
                ::lrit::ImageStructureRecord image_structure_record = file.getHeader<::lrit::ImageStructureRecord>();

                // Parse compression headers
                if (image_structure_record.compression_flag == 2 /* Progressive JPEG */)
                {
                    logger->debug("JPEG Compression is used, decompressing...");
                    file.custom_flags.insert_or_assign(JPEG_COMPRESSED, true);
                    file.custom_flags.insert_or_assign(WT_COMPRESSED, false);
                }
                else if (image_structure_record.compression_flag == 1 /* Wavelet */)
                {
                    logger->debug("Wavelet Compression is used, decompressing...");
                    file.custom_flags.insert_or_assign(JPEG_COMPRESSED, false);
                    file.custom_flags.insert_or_assign(WT_COMPRESSED, true);
                }
                else
                {
                    file.custom_flags.insert_or_assign(JPEG_COMPRESSED, false);
                    file.custom_flags.insert_or_assign(WT_COMPRESSED, false);
                }

                // Decompress
                if (file.custom_flags[JPEG_COMPRESSED]) // Is this Jpeg-Compressed? Decompress. MSG only allows 8-bits
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
                    // This could be better... Sometimes I should maybe adapt the DecompWT code to be cleaner for this purpose...
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
                }
            }
        } // namespace msg
    } // namespace xrit
} // namespace satdump