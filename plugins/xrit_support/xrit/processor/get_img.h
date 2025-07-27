#pragma once

#include "common/lrit/lrit_file.h"
#include "image/image.h"
#include "logger.h"
#include "xrit/identify.h"

namespace satdump
{
    namespace xrit
    {
        inline image::Image getImageFromXRITFile(xrit_file_type_t type, lrit::LRITFile &file)
        {
            ::lrit::PrimaryHeader primary_header = file.getHeader<::lrit::PrimaryHeader>();

            if (!file.hasHeader<lrit::ImageStructureRecord>())
            {
                logger->error("Tried to parse XRIT image but no image header!");
                return image::Image();
            }

            ::lrit::ImageStructureRecord image_structure_record = file.getHeader<::lrit::ImageStructureRecord>(); //(&lrit_data[all_headers[ImageStructureRecord::TYPE]]);

            size_t required_buffer_size = (image_structure_record.bit_per_pixel > 8 ? 2 : 1) * image_structure_record.columns_count * image_structure_record.lines_count;
            if (required_buffer_size <= file.lrit_data.size() - primary_header.total_header_length)
            {
                image::Image image(&file.lrit_data[primary_header.total_header_length], image_structure_record.bit_per_pixel > 8 ? 16 : 8, image_structure_record.columns_count,
                                   image_structure_record.lines_count, 1);

                if (type == XRIT_HIMAWARI_AHI)
                {
                    if (image_structure_record.bit_per_pixel == 16)
                    {
                        // Special case as Himawari sends it in Big Endian...?
                        image::Image image2(16, image_structure_record.columns_count, image_structure_record.lines_count, 1);

                        for (long long int i = 0; i < image_structure_record.columns_count * image_structure_record.lines_count; i++)
                            image2.set(i, ((&file.lrit_data[primary_header.total_header_length])[i * 2 + 0] << 8 | (&file.lrit_data[primary_header.total_header_length])[i * 2 + 1]));

                        image = image2;

                        // Needs to be shifted up by 4
                        for (long long int i = 0; i < image_structure_record.columns_count * image_structure_record.lines_count; i++)
                            image.set(i, image.get(i) << 6);
                    }
                }

                return image;
            }
            else
            {
                logger->error("File is an image but the file is too small!");
                return image::Image();
            }
        }

    } // namespace xrit
} // namespace satdump