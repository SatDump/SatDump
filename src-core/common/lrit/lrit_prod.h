#pragma once

#include <string>
#include <ctime>
#include <thread>
#include <filesystem>
#include <mutex>
#include "products/image_products.h"
#include "lrit_file.h"
#include "logger.h"

/////
#include "common/utils.h"

namespace lrit
{
    inline std::string getLRITImageFilename(double timestamp, std::string sat_name, std::string channel)
    {
        time_t tt_n = timestamp;
        std::tm *timeReadable = gmtime(&tt_n);
        std::string utc_filename = sat_name + "_" + channel + "_" +                                                                                             // Satellite name and channel
                                   std::to_string(timeReadable->tm_year + 1900) +                                                                               // Year yyyy
                                   (timeReadable->tm_mon + 1 > 9 ? std::to_string(timeReadable->tm_mon + 1) : "0" + std::to_string(timeReadable->tm_mon + 1)) + // Month MM
                                   (timeReadable->tm_mday > 9 ? std::to_string(timeReadable->tm_mday) : "0" + std::to_string(timeReadable->tm_mday)) + "T" +    // Day dd
                                   (timeReadable->tm_hour > 9 ? std::to_string(timeReadable->tm_hour) : "0" + std::to_string(timeReadable->tm_hour)) +          // Hour HH
                                   (timeReadable->tm_min > 9 ? std::to_string(timeReadable->tm_min) : "0" + std::to_string(timeReadable->tm_min)) +             // Minutes mm
                                   (timeReadable->tm_sec > 9 ? std::to_string(timeReadable->tm_sec) : "0" + std::to_string(timeReadable->tm_sec)) + "Z";        // Seconds ss
        return utc_filename;
    }

    inline double stringTimestampToDouble(const char *strptimestr, std::string timestamp_str)
    {
        std::tm scanTimestamp;
        scanTimestamp.tm_sec = 0; // No seconds
        strptime(timestamp_str.c_str(), strptimestr, &scanTimestamp);
        return timegm(&scanTimestamp);
    }

    inline nlohmann::json convertNavigationHeaderToProjCfg(ImageNavigationRecord record, int width)
    {
        logger->trace("------------------------------------------------------- %s - %d %d - %d %d",
                      record.projection_name,
                      record.column_scaling_factor, record.line_scaling_factor,
                      record.column_offset, record.line_offset);
        if ((record.projection_name[0] == 'G' &&
             record.projection_name[1] == 'E' &&
             record.projection_name[2] == 'O' &&
             record.projection_name[3] == 'S') ||
            (record.projection_name[0] == 'g' &&
             record.projection_name[1] == 'e' &&
             record.projection_name[2] == 'o' &&
             record.projection_name[3] == 's'))
        {
            float position = 0;
            if (sscanf(record.projection_name, "GEOS(%f)", &position) == 1 ||
                sscanf(record.projection_name, "geos(%f)", &position) == 1)
            {
                nlohmann::json cfg;
#if 0
                cfg["type"] = "geos";
                cfg["lon"] = position;
                cfg["alt"] = 35786;
                cfg["scale_x"] = 1.145;
                cfg["scale_y"] = 1.145;
                cfg["offset_x"] = 0.0003;
                cfg["offset_y"] = 0.0003;
                cfg["sweep_x"] = false;
#else
                cfg["type"] = "geos_xrit";
                cfg["width"] = width;
                cfg["sat_lon"] = position;
                cfg["scale_x"] = record.column_scaling_factor;
                cfg["scale_y"] = record.line_scaling_factor;
                cfg["offset_x"] = record.column_offset;
                cfg["offset_y"] = record.line_offset;
#endif
                return cfg;
            }
            else
                return "";
        }
        else
            return "";
    }

    struct ImageDescription
    {
        std::string sat_name; // Satellite name
        std::string cha_name; // Channel name
        double timestamp;     // Timestamp, UTC
        std::string directory;
        nlohmann::json proj_cfg;
    };

    class xRITProductizer
    {
    private:
        //
        std::string d_name;
        time_t last_update_time = 0;

        //
        bool thread_should_run = true;
        std::thread d_thread;
        std::mutex gbd_mtx;

        void background_thread()
        {
            while (thread_should_run)
            {
                gbd_mtx.lock();
                if (last_update_time != 0)
                {
                    if (time(0) - last_update_time > 240)
                        save_product();
                }
                gbd_mtx.unlock();
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }

    private:
        double current_timestamp = -1;
        std::vector<std::string> current_images;
        std::vector<ImageDescription> current_imgdecs;

    private:
        void save_product()
        {
            if (current_images.size() == 0)
                return;

            satdump::ImageProducts xrit_products;
            xrit_products.instrument_name = d_name;
            xrit_products.has_timestamps = false;
            xrit_products.bit_depth = 8;

            if (current_imgdecs[0].proj_cfg != "")
                xrit_products.set_proj_cfg(current_imgdecs[0].proj_cfg);

            for (int i = 0; i < (int)current_images.size(); i++)
            {
                std::string final_path_rel = std::filesystem::relative(d_current_path, std::filesystem::path(current_images[i]).parent_path()).generic_string() +
                                             "/" + std::filesystem::path(current_images[i]).stem().string() + std::filesystem::path(current_images[i]).extension().string();
                // printf("%s\n", final_path_rel.c_str());
                xrit_products.images.push_back({final_path_rel, current_imgdecs[i].cha_name, image::Image<uint16_t>()});
                logger->critical("IMAGE+" + final_path_rel);
            }

            std::string path = d_current_path + "/" + getLRITImageFilename(current_timestamp, current_imgdecs[0].sat_name, "product") + ".cbor";
            logger->critical(path);
            xrit_products.save("", false, path);

            current_images.clear();
            current_imgdecs.clear();
            current_timestamp = -1;
            is_saved = true;
        }

    public:
        xRITProductizer(std::string name)
            : d_name(name)
        {
            d_thread = std::thread(&xRITProductizer::background_thread, this);
        }

        ~xRITProductizer()
        {
            thread_should_run = false;
            if (d_thread.joinable())
                d_thread.join();
            save_product();
        }

        void push_image(std::string image_path, ImageDescription image_desc)
        {
            gbd_mtx.lock();
            if (image_desc.timestamp != current_timestamp)
                save_product();

            current_images.push_back(image_path);
            current_imgdecs.push_back(image_desc);
            current_timestamp = image_desc.timestamp;

            last_update_time = time(0);
            is_saved = false;
            gbd_mtx.unlock();
        }

        void tick()
        {
            gbd_mtx.lock();
            last_update_time = time(0);
            gbd_mtx.unlock();
        }

        //
        std::string d_current_path;
        bool is_saved = false;
    };

    template <typename T>
    inline std::string save_lrit_image(image::Image<T> &img, ImageDescription desc, xRITProductizer *productizer = nullptr)
    {
        std::string path = desc.directory + "/" + getLRITImageFilename(desc.timestamp, desc.sat_name, desc.cha_name);
        img.append_ext(&path);
        img.save_img(path);
        if (productizer != nullptr)
            productizer->push_image(path, desc);
        return path;
    }
}