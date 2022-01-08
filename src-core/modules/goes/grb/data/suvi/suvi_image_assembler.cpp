#include "suvi_image_assembler.h"
#include "logger.h"
#include <filesystem>

namespace goes
{
    namespace grb
    {
        GRBSUVIImageAssembler::GRBSUVIImageAssembler(std::string suvi_dir, products::SUVI::GRBProductSUVI config)
            : suvi_directory(suvi_dir),
              suvi_product(config),
              currentTimeStamp(0)
        {
            hasImage = false;
        }

        GRBSUVIImageAssembler::~GRBSUVIImageAssembler()
        {
            if (hasImage)
                save();
        }

        void GRBSUVIImageAssembler::save()
        {
            time_t time_tt = currentTimeStamp;
            std::tm *timeReadable = gmtime(&time_tt);
            std::string utc_filename = std::to_string(timeReadable->tm_year + 1900) +                                                                               // Year yyyy
                                       (timeReadable->tm_mon + 1 > 9 ? std::to_string(timeReadable->tm_mon + 1) : "0" + std::to_string(timeReadable->tm_mon + 1)) + // Month MM
                                       (timeReadable->tm_mday > 9 ? std::to_string(timeReadable->tm_mday) : "0" + std::to_string(timeReadable->tm_mday)) + "T" +    // Day dd
                                       (timeReadable->tm_hour > 9 ? std::to_string(timeReadable->tm_hour) : "0" + std::to_string(timeReadable->tm_hour)) +          // Hour HH
                                       (timeReadable->tm_min > 9 ? std::to_string(timeReadable->tm_min) : "0" + std::to_string(timeReadable->tm_min)) +             // Minutes mm
                                       (timeReadable->tm_sec > 9 ? std::to_string(timeReadable->tm_sec) : "0" + std::to_string(timeReadable->tm_sec)) + "Z";

            std::string filename = "SUVI_" + suvi_product.channel + "_" + utc_filename + ".png";
            std::string directory = suvi_directory + "/" + suvi_product.channel + "/";
            std::filesystem::create_directories(directory);

            logger->info("Saving " + directory + filename);
            full_image.save_png(std::string(directory + filename).c_str());
        }

        void GRBSUVIImageAssembler::reset()
        {
            int image_width = suvi_product.width;
            int image_height = suvi_product.height;

            full_image = image::Image<uint16_t>(image_width, image_height, 1);
            full_image.fill(0);
            hasImage = false;
        }

        void GRBSUVIImageAssembler::pushBlock(GRBImagePayloadHeader header, image::Image<uint16_t> &block)
        {
            // Check this is the same image
            if (currentTimeStamp != header.utc_time)
            {
                if (hasImage)
                    save();
                reset();

                currentTimeStamp = header.utc_time;
                hasImage = true;
            }

            // Scale image to full bit depth
            //block <<= 2;

            // Fill
            full_image.draw_image(0, block, header.left_x_coord, header.left_y_coord + header.row_offset_image_block);
        }
    }
}