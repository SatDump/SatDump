#include "lrit_productizer.h"
#include "products/image_products.h"
#include "common/utils.h"
#include <filesystem>
#include "logger.h"

namespace lrit
{
    inline std::string getXRITTimestamp(time_t tim)
    {
        std::tm *timeReadable = gmtime(&tim);
        std::string utc_filename =
            std::to_string(timeReadable->tm_year + 1900) +                                                                               // Year yyyy
            (timeReadable->tm_mon + 1 > 9 ? std::to_string(timeReadable->tm_mon + 1) : "0" + std::to_string(timeReadable->tm_mon + 1)) + // Month MM
            (timeReadable->tm_mday > 9 ? std::to_string(timeReadable->tm_mday) : "0" + std::to_string(timeReadable->tm_mday)) + "T" +    // Day dd
            (timeReadable->tm_hour > 9 ? std::to_string(timeReadable->tm_hour) : "0" + std::to_string(timeReadable->tm_hour)) +          // Hour HH
            (timeReadable->tm_min > 9 ? std::to_string(timeReadable->tm_min) : "0" + std::to_string(timeReadable->tm_min)) +             // Minutes mm
            (timeReadable->tm_sec > 9 ? std::to_string(timeReadable->tm_sec) : "0" + std::to_string(timeReadable->tm_sec)) + "Z";        // Seconds ss
        return utc_filename;
    }

    std::string timestamp_to_string2(double timestamp)
    {
        if (timestamp < 0)
            timestamp = 0;
        time_t tttime = timestamp;
        std::tm *timeReadable = gmtime(&tttime);
        return std::to_string(timeReadable->tm_year + 1900) + "-" +
               (timeReadable->tm_mon + 1 > 9 ? std::to_string(timeReadable->tm_mon + 1) : "0" + std::to_string(timeReadable->tm_mon + 1)) + "-" +
               (timeReadable->tm_mday > 9 ? std::to_string(timeReadable->tm_mday) : "0" + std::to_string(timeReadable->tm_mday)) + "_" +
               (timeReadable->tm_hour > 9 ? std::to_string(timeReadable->tm_hour) : "0" + std::to_string(timeReadable->tm_hour)) + "-" +
               (timeReadable->tm_min > 9 ? std::to_string(timeReadable->tm_min) : "0" + std::to_string(timeReadable->tm_min)) + "-" +
               (timeReadable->tm_sec > 9 ? std::to_string(timeReadable->tm_sec) : "0" + std::to_string(timeReadable->tm_sec));
    }

    template <typename T>
    LRITProductizer<T>::LRITProductizer()
    {
    }

    template <typename T>
    LRITProductizer<T>::~LRITProductizer()
    {
    }

    template <typename T>
    void LRITProductizer<T>::saveImage(image::Image<T> img,
                                       std::string directory,
                                       std::string satellite,
                                       std::string satshort,
                                       std::string channel,
                                       time_t timestamp,
                                       std::string region,
                                       lrit::ImageNavigationRecord *image_navigation_record)
    {
        std::string directory_path = region == ""
                                         ? (directory + "/" + satellite + "/" + timestamp_to_string2(timestamp) + "/")
                                         : (directory + "/" + satellite + "/" + region + "/" + timestamp_to_string2(timestamp) + "/");
        std::string filename = satshort + "_" + channel + "_" + getXRITTimestamp(timestamp) + ".png";
        std::string pro_f_path = directory_path + "product.cbor";

        if (!std::filesystem::exists(directory_path))
            std::filesystem::create_directories(directory_path);

        nlohmann::json proj_cfg;
        if (image_navigation_record)
        {
            float sat_pos = 0;
            if (sscanf(image_navigation_record->projection_name.c_str(), "geos(%f)", &sat_pos) == 1)
            {
                constexpr float k = 624597.0334223134;
                double scalar_x = double(image_navigation_record->column_scaling_factor) * pow(2, -16) * 6.433;
                double scalar_y = double(image_navigation_record->line_scaling_factor) * pow(2, -16) * 6.433;
                proj_cfg["type"] = "geos";
                proj_cfg["lon0"] = sat_pos;
                proj_cfg["sweep_x"] = true;
                proj_cfg["scalar_x"] = scalar_x;
                proj_cfg["scalar_y"] = -scalar_y;
                proj_cfg["offset_x"] = double(image_navigation_record->column_offset) * -scalar_x;
                proj_cfg["offset_y"] = double(image_navigation_record->line_offset) * scalar_y;
                proj_cfg["width"] = img.width();
                proj_cfg["height"] = img.height();
                proj_cfg["altitude"] = 35786023.00;
            }
        }

        // Check if the image already exists, this can happen when GOES goes (laugh please, pretty please) wrong.
        // If it does, we append a number and do NOT overwrite.
        if (std::filesystem::exists(directory_path + filename))
        {
            int current_iteration = 1;
            std::string filename_new;
            do
            {
                filename_new = satshort + "_" + channel + "_" + getXRITTimestamp(timestamp) + "_" + std::to_string(current_iteration++) + ".png";
            } while (std::filesystem::exists(directory_path + filename_new));
            img.save_png(directory_path + filename_new);
            logger->warn("Image already existed. Written as %s", filename_new.c_str());
        }
        // Otherwise, we can go on as usual and write a proper product.
        else
        {
            // If the product file already exists, we want to append the new
            // channel to it, without overwriting anything.
            // Products are loaded without loading images.
            if (std::filesystem::exists(pro_f_path))
            {
                satdump::ImageProducts pro;
                pro.d_no_not_save_images = true;
                pro.d_no_not_load_images = true;
                pro.load(pro_f_path);
                bool contains = false;
                for (auto &img : pro.images)
                    if (img.channel_name == channel)
                        contains = true;
                if (!contains)
                    pro.images.push_back({filename, channel, image::Image<uint16_t>()});
                if (!pro.has_proj_cfg())
                {
                    if (proj_cfg.size() > 0)
                    {
                        pro.set_proj_cfg(proj_cfg);
                        logger->critical("\n%s\n", proj_cfg.dump(4).c_str());
                    }
                }
                else
                {
                    if (pro.get_proj_cfg().contains("width") && proj_cfg.contains("width"))
                        if (proj_cfg["width"].get<int>() > pro.get_proj_cfg()["width"].get<int>())
                            pro.set_proj_cfg(proj_cfg);
                }
                pro.save(directory_path);
            }
            // Otherwise, create a new product with the current single channel we have.
            else
            {
                satdump::ImageProducts pro;
                pro.d_no_not_save_images = true;
                pro.instrument_name = "abi";
                pro.bit_depth = sizeof(T) * 8;
                if (proj_cfg.size() > 0)
                {
                    pro.set_proj_cfg(proj_cfg);
                    logger->critical("\n%s\n", proj_cfg.dump(4).c_str());
                }
                pro.images.push_back({filename, channel, image::Image<uint16_t>()});
                pro.save(directory_path);
            }
        }

        img.save_png(directory_path + filename);
    }

    template class LRITProductizer<uint8_t>;
    template class LRITProductizer<uint16_t>;
}