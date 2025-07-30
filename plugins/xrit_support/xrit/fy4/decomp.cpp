#include "decomp.h"
#include "image/j2k_utils.h"
#include "image/jpeg12_utils.h"
#include "image/jpeg_utils.h"
#include "logger.h"

namespace satdump
{
    namespace xrit
    {
        namespace fy4
        {
            void decompressFY4HritFileIfRequired(XRITFile &file)
            {
                PrimaryHeader primary_header = file.getHeader<PrimaryHeader>();

                // Only image data can be compressed
                if (primary_header.file_type_code != 0 || !file.hasHeader<ImageStructureRecord>())
                {
                    logger->error("File " + file.filename + " is not an image!");
                    return;
                }

                ImageInformationRecord image_structure_record = file.getHeader<ImageInformationRecord>();

                if (true) // file.custom_flags[JPG_COMPRESSED] || file.custom_flags[J2K_COMPRESSED]) // Is this Jpeg-Compressed? Decompress
                {
                    logger->info("Decompressing J2K... (Type %d, Depth %d)", image_structure_record.compressed_info_algo, image_structure_record.bit_per_pixel);

                    int offset = 0;
                    {
                        uint8_t shifter[4] = {0, 0, 0, 0};
                        for (int i = primary_header.total_header_length; i < (int)file.lrit_data.size(); i++)
                        {
                            shifter[0] = shifter[1];
                            shifter[1] = shifter[2];
                            shifter[2] = shifter[3];
                            shifter[3] = file.lrit_data[i];

                            if (shifter[0] == 0xFF && shifter[1] == 0x4F && shifter[2] == 0xFF && shifter[3] == 0x51)
                            {
                                offset = i - primary_header.total_header_length - 3;
                                break;
                            }
                        }
                    }

                    logger->critical("OFFSET %d", offset);

                    image::Image img = image::decompress_j2k_openjp2(&file.lrit_data[primary_header.total_header_length + offset], //
                                                                     file.lrit_data.size() - primary_header.total_header_length - offset);

                    // if (image_structure_record.bit_per_pixel <= 8)
                    // {
                    //     image::Image img2(8, img.width(), img.height(), 1);
                    //     for (size_t i = 0; i < img.size(); i++)
                    //         img2.set(i, img2.get(i));
                    //     img = img2;
                    // }

                    if (img.width() < image_structure_record.columns_count || img.height() < image_structure_record.lines_count)
                    {
                        img.init(image_structure_record.bit_per_pixel > 8 ? 16 : 8, image_structure_record.columns_count, image_structure_record.lines_count, 1); // Just in case it's corrupted!
                        logger->error("Error decompressing FY-4x image!");
                    }

                    file.lrit_data.erase(file.lrit_data.begin() + primary_header.total_header_length, file.lrit_data.end());
                    if (image_structure_record.bit_per_pixel <= 8)
                        file.lrit_data.insert(file.lrit_data.end(), (uint8_t *)img.raw_data(), (uint8_t *)img.raw_data() + img.height() * img.width());
                    else
                        file.lrit_data.insert(file.lrit_data.end(), (uint8_t *)img.raw_data(), (uint8_t *)img.raw_data() + img.height() * img.width() * 2);
                }
            }
        } // namespace fy4
    } // namespace xrit
} // namespace satdump