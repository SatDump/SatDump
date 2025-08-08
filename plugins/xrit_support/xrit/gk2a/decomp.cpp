#include "decomp.h"
#include "image/j2k_utils.h"
#include "image/jpeg12_utils.h"
#include "image/jpeg_utils.h"
#include "logger.h"
#include <fstream>

namespace satdump
{
    namespace xrit
    {
        namespace gk2a
        {
            void decompressGK2AHritFileIfRequired(XRITFile &file)
            {
                PrimaryHeader primary_header = file.getHeader<PrimaryHeader>();

                // Only image data can be compressed
                if (primary_header.file_type_code != 0 || !file.hasHeader<ImageStructureRecord>())
                {
                    logger->error("File " + file.filename + " is not an image!");
                    return;
                }

                ImageStructureRecord image_structure_record = file.getHeader<ImageStructureRecord>(); //(&lrit_data[all_headers[ImageStructureRecord::TYPE]]);
                logger->debug("This is image data. Size " + std::to_string(image_structure_record.columns_count) + "x" + std::to_string(image_structure_record.lines_count));
                if (image_structure_record.compression_flag == 2 /* Progressive JPEG */)
                {
                    logger->debug("JPEG Compression is used, decompressing...");
                    file.custom_flags.insert_or_assign(JPG_COMPRESSED, true);
                    file.custom_flags.insert_or_assign(J2K_COMPRESSED, false);
                }
                else if (image_structure_record.compression_flag == 1 /* Wavelet */)
                {
                    logger->debug("Wavelet Compression is used, decompressing...");
                    file.custom_flags.insert_or_assign(JPG_COMPRESSED, false);
                    file.custom_flags.insert_or_assign(J2K_COMPRESSED, true);
                }
                else
                {
                    file.custom_flags.insert_or_assign(JPG_COMPRESSED, false);
                    file.custom_flags.insert_or_assign(J2K_COMPRESSED, false);
                }

                if (file.custom_flags[JPG_COMPRESSED] || file.custom_flags[J2K_COMPRESSED]) // Is this Jpeg-Compressed? Decompress
                {
                    logger->info("Decompressing JPEG (%d)...", image_structure_record.bit_per_pixel);
                    image::Image img;
                    if (image_structure_record.bit_per_pixel > 8)
                    {
                        img = image::decompress_jpeg12(&file.lrit_data[primary_header.total_header_length], file.lrit_data.size() - primary_header.total_header_length);
                        for (size_t c = 0; c < img.size(); c++)
                            img.set(c, img.get(c) << 2);
                    }
                    else
                        img = image::decompress_jpeg(&file.lrit_data[primary_header.total_header_length], file.lrit_data.size() - primary_header.total_header_length);

                    if (img.size() == 0) // UHRIT is offset apparently?
                    {
                        img = image::decompress_j2k_openjp2(&file.lrit_data[primary_header.total_header_length + 85], file.lrit_data.size() - primary_header.total_header_length - 85);
                        for (size_t c = 0; c < img.size(); c++)
                            img.set(c, img.get(c) << (16 - image_structure_record.bit_per_pixel));
                    }

                    if (img.width() < image_structure_record.columns_count || img.height() < image_structure_record.lines_count)
                    {
                        logger->warn("JPEG decomp error! %d", img.size());
                        img.init(image_structure_record.bit_per_pixel > 8 ? 16 : 8, image_structure_record.columns_count, image_structure_record.lines_count, 1); // Just in case it's corrupted!
                    }
                    file.lrit_data.erase(file.lrit_data.begin() + primary_header.total_header_length, file.lrit_data.end());
                    file.lrit_data.insert(file.lrit_data.end(), (uint8_t *)img.raw_data(), (uint8_t *)img.raw_data() + img.size() * img.typesize());
                }
            }
        } // namespace gk2a
    } // namespace xrit
} // namespace satdump