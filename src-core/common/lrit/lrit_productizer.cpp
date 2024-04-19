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
    LRITProductizer<T>::LRITProductizer(std::string instrument_id, bool sweep_x) : instrument_id(instrument_id), should_sweep_x(sweep_x)
    {
    }

    template <typename T>
    LRITProductizer<T>::~LRITProductizer()
    {
    }

    // This will most probably get moved over to each
    inline void addCalibrationInfoFunc(satdump::ImageProducts &pro, ImageDataFunctionRecord *image_data_function_record, std::string channel, std::string satellite, std::string instrument_id)
    {
        const float goesn_imager_wavelength_table[5] = {
            630,
            3900,
            64800,
            10700,
            13300,
        };

        const float goes_abi_wavelength_table[16] = {
            470,
            640,
            860,
            1380,
            1610,
            2260,
            3900,
            6190,
            6950,
            7340,
            850,
            9610,
            10350,
            11200,
            12300,
            13300,
        };

        const float hima_ahi_wavelength_table[16] = {
            470,
            510,
            640,
            860,
            1600,
            2300,
            3900,
            6200,
            6900,
            7300,
            8600,
            9600,
            10400,
            11200,
            12400,
            13300,
        };

        if (image_data_function_record)
        {
            // Default, to not crash the viewer on no calib
            pro.set_calibration_type(pro.images.size() - 1, pro.CALIB_RADIANCE);

            if (instrument_id == "goesn_imager" && std::stoi(channel) > 0 && std::stoi(channel) <= 5)
                pro.set_wavenumber(pro.images.size() - 1, 1e7 / goesn_imager_wavelength_table[std::stoi(channel) - 1]);
            else if (instrument_id == "abi" && std::stoi(channel) > 0 && std::stoi(channel) <= 16)
                pro.set_wavenumber(pro.images.size() - 1, 1e7 / goes_abi_wavelength_table[std::stoi(channel) - 1]);
            else if (instrument_id == "ahi" && std::stoi(channel) > 0 && std::stoi(channel) <= 16)
                pro.set_wavenumber(pro.images.size() - 1, 1e7 / hima_ahi_wavelength_table[std::stoi(channel) - 1]);
            else
                pro.set_wavenumber(pro.images.size() - 1, -1);

            auto lines = splitString(image_data_function_record->datas, '\n');
            if (lines[0] == "$HALFTONE:=8")
            {
                if (lines[1] == "_NAME:=toa_lambertian_equivalent_albedo_multiplied_by_cosine_solar_zenith_angle")
                {
                    nlohmann::json lut;
                    for (int i = 3; i < lines.size(); i++)
                    {
                        int val;
                        float valo;
                        if (sscanf(lines[i].c_str(), "%d:=%f", &val, &valo) == 2)
                            lut[val] = valo;
                    }

                    nlohmann::json calib_cfg = pro.get_calibration_raw();

                    if (satellite.find("GOES-") != std::string::npos || satellite == "Himawari")
                        calib_cfg["calibrator"] = "goes_xrit";

                    calib_cfg[channel] = lut;
                    pro.set_calibration(calib_cfg);
                    pro.set_calibration_type(pro.images.size() - 1, pro.CALIB_REFLECTANCE);
                }
                else if (lines[1] == "_NAME:=toa_brightness_temperature")
                {
                    nlohmann::json lut;
                    for (int i = 3; i < lines.size(); i++)
                    {
                        int val;
                        float valo;
                        if (sscanf(lines[i].c_str(), "%d:=%f", &val, &valo) == 2)
                            lut[val] = valo;
                    }

                    nlohmann::json calib_cfg = pro.get_calibration_raw();

                    if (satellite.find("GOES-") != std::string::npos || satellite == "Himawari")
                        calib_cfg["calibrator"] = "goes_xrit";

                    calib_cfg[channel] = lut;
                    pro.set_calibration(calib_cfg);
                    pro.set_calibration_type(pro.images.size() - 1, pro.CALIB_RADIANCE);
                }
            }
        }
    }

    template <typename T>
    void LRITProductizer<T>::saveImage(image::Image<T> img,
                                       std::string directory,
                                       std::string satellite,
                                       std::string satshort,
                                       std::string channel,
                                       time_t timestamp,
                                       std::string region,
                                       lrit::ImageNavigationRecord *image_navigation_record,
                                       ImageDataFunctionRecord *image_data_function_record)
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
            if (sscanf(image_navigation_record->projection_name.c_str(), "geos(%f)", &sat_pos) == 1 ||
                sscanf(image_navigation_record->projection_name.c_str(), "GEOS(%f)", &sat_pos) == 1)
            {
                constexpr float k = 624597.0334223134;
                double scalar_x = (pow(2, 16) / double(image_navigation_record->column_scaling_factor)) * k;
                double scalar_y = (pow(2, 16) / double(image_navigation_record->line_scaling_factor)) * k;
                proj_cfg["type"] = "geos";
                proj_cfg["lon0"] = sat_pos;
                proj_cfg["sweep_x"] = should_sweep_x;
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

                addCalibrationInfoFunc(pro, image_data_function_record, channel, satellite, instrument_id);

                pro.save(directory_path);
            }
            // Otherwise, create a new product with the current single channel we have.
            else
            {
                satdump::ImageProducts pro;
                pro.d_no_not_save_images = true;
                pro.instrument_name = instrument_id;
                pro.bit_depth = sizeof(T) * 8;
                if (proj_cfg.size() > 0)
                {
                    pro.set_proj_cfg(proj_cfg);
                    logger->critical("\n%s\n", proj_cfg.dump(4).c_str());
                }
                pro.images.push_back({filename, channel, image::Image<uint16_t>()});

                addCalibrationInfoFunc(pro, image_data_function_record, channel, satellite, instrument_id);

                pro.save(directory_path);
            }
        }

        img.save_png(directory_path + filename);
    }

    template class LRITProductizer<uint8_t>;
    template class LRITProductizer<uint16_t>;
}