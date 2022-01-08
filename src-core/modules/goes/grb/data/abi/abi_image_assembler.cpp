#include "abi_image_assembler.h"
#include "logger.h"
#include <filesystem>

namespace goes
{
    namespace grb
    {
        GRBABIImageAssembler::GRBABIImageAssembler(std::string abi_dir, products::ABI::GRBProductABI config)
            : abi_directory(abi_dir),
              abi_product(config),
              currentTimeStamp(0)
        {
            hasImage = false;
        }

        GRBABIImageAssembler::~GRBABIImageAssembler()
        {
            if (hasImage)
                save();
        }

        void GRBABIImageAssembler::save()
        {
            std::string zone = products::ABI::abiZoneToString(abi_product.type);
            time_t time_tt = currentTimeStamp;
            std::tm *timeReadable = gmtime(&time_tt);
            std::string utc_filename = std::to_string(timeReadable->tm_year + 1900) +                                                                               // Year yyyy
                                       (timeReadable->tm_mon + 1 > 9 ? std::to_string(timeReadable->tm_mon + 1) : "0" + std::to_string(timeReadable->tm_mon + 1)) + // Month MM
                                       (timeReadable->tm_mday > 9 ? std::to_string(timeReadable->tm_mday) : "0" + std::to_string(timeReadable->tm_mday)) + "T" +    // Day dd
                                       (timeReadable->tm_hour > 9 ? std::to_string(timeReadable->tm_hour) : "0" + std::to_string(timeReadable->tm_hour)) +          // Hour HH
                                       (timeReadable->tm_min > 9 ? std::to_string(timeReadable->tm_min) : "0" + std::to_string(timeReadable->tm_min)) +             // Minutes mm
                                       (timeReadable->tm_sec > 9 ? std::to_string(timeReadable->tm_sec) : "0" + std::to_string(timeReadable->tm_sec)) + "Z";

            std::string filename = "ABI_" + zone + "_" + std::to_string(abi_product.channel) + "_" + utc_filename + ".png";
            std::string directory = abi_directory + "/" + zone + "/" + utc_filename + "/";
            std::filesystem::create_directories(directory);

            logger->info("Saving " + directory + filename);
            full_image.save_png(std::string(directory + filename).c_str());
        }

        void GRBABIImageAssembler::reset()
        {
            int image_width = 20000;
            int image_height = 20000;

            if (abi_product.type == products::ABI::FULL_DISK)
            {
                image_width = double(ABI_FULLDISK_MODE_WIDTH) / products::ABI::ABI_CHANNEL_PARAMS[abi_product.channel].resolution;
                image_height = double(ABI_FULLDISK_MODE_HEIGHT) / products::ABI::ABI_CHANNEL_PARAMS[abi_product.channel].resolution;
            }
            else if (abi_product.type == products::ABI::CONUS)
            {
                image_width = double(ABI_CONUS_MODE_WIDTH) / products::ABI::ABI_CHANNEL_PARAMS[abi_product.channel].resolution;
                image_height = double(ABI_CONUS_MODE_HEIGHT) / products::ABI::ABI_CHANNEL_PARAMS[abi_product.channel].resolution;
            }
            else if (abi_product.type == products::ABI::MESO_1 || abi_product.type == products::ABI::MESO_2)
            {
                image_width = double(ABI_MESO_MODE_WIDTH) / products::ABI::ABI_CHANNEL_PARAMS[abi_product.channel].resolution;
                image_height = double(ABI_MESO_MODE_HEIGHT) / products::ABI::ABI_CHANNEL_PARAMS[abi_product.channel].resolution;
            }

            full_image = image::Image<uint16_t>(image_width, image_height, 1);
            full_image.fill(0);
            hasImage = false;
        }

        void GRBABIImageAssembler::pushBlock(GRBImagePayloadHeader header, image::Image<uint16_t> &block)
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
            block <<= 16 - products::ABI::ABI_CHANNEL_PARAMS[abi_product.channel].bit_depth;

            // Fill
            full_image.draw_image(0, block, header.left_x_coord, header.left_y_coord + header.row_offset_image_block);
        }
    }
}