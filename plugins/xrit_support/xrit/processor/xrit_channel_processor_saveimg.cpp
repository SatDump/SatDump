#include "common/lrit/lrit_file.h"
#include "image/io.h"
#include "image/processing.h"
#include "logger.h"
#include "nlohmann/json.hpp"
#include "products/image_product.h"
#include "utils/string.h"
#include "xrit/identify.h"
#include "xrit_channel_processor.h"
#include <filesystem>
#include <string>

namespace satdump
{
    namespace xrit
    {
        // TODOREWORK move those over??
        std::string getXRITTimestamp(time_t tim)
        {
            std::tm *timeReadable = gmtime(&tim);
            std::string utc_filename = std::to_string(timeReadable->tm_year + 1900) +                                                                               // Year yyyy
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
            return std::to_string(timeReadable->tm_year + 1900) + "-" + (timeReadable->tm_mon + 1 > 9 ? std::to_string(timeReadable->tm_mon + 1) : "0" + std::to_string(timeReadable->tm_mon + 1)) +
                   "-" + (timeReadable->tm_mday > 9 ? std::to_string(timeReadable->tm_mday) : "0" + std::to_string(timeReadable->tm_mday)) + "_" +
                   (timeReadable->tm_hour > 9 ? std::to_string(timeReadable->tm_hour) : "0" + std::to_string(timeReadable->tm_hour)) + "-" +
                   (timeReadable->tm_min > 9 ? std::to_string(timeReadable->tm_min) : "0" + std::to_string(timeReadable->tm_min)) + "-" +
                   (timeReadable->tm_sec > 9 ? std::to_string(timeReadable->tm_sec) : "0" + std::to_string(timeReadable->tm_sec));
        }

        // This will most probably get moved over to each
        inline void addCalibrationInfoFunc(satdump::products::ImageProduct &pro, lrit::ImageDataFunctionRecord *image_data_function_record, std::string channel, std::string instrument_id)
        {
            if (image_data_function_record)
            {
                // Default, to not crash the explorer on no calib TODOREWORK not needed?
                // pro.set_calibration_type(pro.images.size() - 1, pro.CALIB_RADIANCE);

                if (instrument_id == "goesn_imager" && std::stoi(channel) > 0 && std::stoi(channel) <= 5)
                {
                    const float goesn_imager_wavelength_table[5] = {
                        630, 3900, 6480, 10700, 13300,
                    };

                    pro.set_channel_wavenumber(pro.images.size() - 1, 1e7 / goesn_imager_wavelength_table[std::stoi(channel) - 1]);
                }
                else if (instrument_id == "abi" && channel.find_first_not_of("0123456789") == std::string::npos && std::stoi(channel) > 0 && std::stoi(channel) <= 16)
                {
                    const float goes_abi_wavelength_table[16] = {
                        470, 640, 860, 1380, 1610, 2260, 3900, 6190, 6950, 7340, 8500, 9610, 10350, 11200, 12300, 13300,
                    };

                    pro.set_channel_wavenumber(pro.images.size() - 1, 1e7 / goes_abi_wavelength_table[std::stoi(channel) - 1]);
                }
                else if (instrument_id == "ahi" && std::stoi(channel) > 0 && std::stoi(channel) <= 16)
                {
                    const float hima_ahi_wavelength_table[16] = {
                        470, 510, 640, 860, 1600, 2300, 3900, 6200, 6900, 7300, 8600, 9600, 10400, 11200, 12400, 13300,
                    };

                    pro.set_channel_wavenumber(pro.images.size() - 1, 1e7 / hima_ahi_wavelength_table[std::stoi(channel) - 1]);
                }
                else if (instrument_id == "ami")
                {
                    double wavelength_nm = std::stod(channel.substr(2, channel.size() - 1)) * 100;
                    pro.set_channel_wavenumber(pro.images.size() - 1, 1e7 / wavelength_nm);
                }
                else
                    pro.set_channel_wavenumber(pro.images.size() - 1, -1);

                // if (instrument_id == "ahi")
                // printf("Channel %s\n%s\n", channel.c_str(), image_data_function_record->datas.c_str());

                auto lines = splitString(image_data_function_record->datas, '\n');
                if (lines.size() < 4)
                {
                    lines = splitString(image_data_function_record->datas, '\r');
                    if (lines.size() < 4)
                    {
                        logger->error("Error parsing calibration info into lines!");
                        return;
                    }
                }

                // for (int i = 0; i < lines.size(); i++)
                //     logger->critical("%d => %s", i, lines[i].c_str());

                if (lines[0].find("HALFTONE:=") != std::string::npos)
                {
                    // GOES xRIT modes
                    if (instrument_id == "goesn_imager" || instrument_id == "abi")
                    {
                        if (lines[1] == "_NAME:=toa_lambertian_equivalent_albedo_multiplied_by_cosine_solar_zenith_angle")
                        {
                            std::vector<std::pair<int, float>> lut;
                            for (size_t i = 3; i < lines.size(); i++)
                            {
                                int val;
                                float valo;
                                if (sscanf(lines[i].c_str(), "%d:=%f", &val, &valo) == 2)
                                    lut.push_back({val, valo});
                            }

                            nlohmann::json calib_cfg = pro.get_calibration_raw();

                            calib_cfg[channel] = lut;
                            pro.set_calibration("generic_xrit", calib_cfg);
                            pro.set_channel_unit(pro.images.size() - 1, CALIBRATION_ID_ALBEDO);
                        }
                        else if (lines[1] == "_NAME:=toa_brightness_temperature")
                        {
                            std::vector<std::pair<int, float>> lut;
                            for (size_t i = 3; i < lines.size(); i++)
                            {
                                int val;
                                float valo;
                                if (sscanf(lines[i].c_str(), "%d:=%f", &val, &valo) == 2)
                                    lut.push_back({val, valo});
                            }

                            nlohmann::json calib_cfg = pro.get_calibration_raw();

                            calib_cfg[channel] = lut;
                            pro.set_calibration("generic_xrit", calib_cfg);
                            pro.set_channel_unit(pro.images.size() - 1, CALIBRATION_ID_EMISSIVE_RADIANCE);
                        }
                    }

                    // GK-2A xRIT modes
                    if (instrument_id == "ami")
                    {
                        int bits_for_calib = 8;
                        if (lines[0].find("HALFTONE:=10") != std::string::npos)
                            bits_for_calib = 10;

                        if (lines[2] == "_UNIT:=ALBEDO(%)")
                        {
                            std::vector<std::pair<int, float>> lut;
                            for (size_t i = 3; i < lines.size(); i++)
                            {
                                int val;
                                float valo;
                                if (sscanf(lines[i].c_str(), "%d:=%f", &val, &valo) == 2)
                                    lut.push_back({val, valo / 100.0});
                            }

                            nlohmann::json calib_cfg = pro.get_calibration_raw();

                            calib_cfg["bits_for_calib"] = bits_for_calib;

                            calib_cfg[channel] = lut;
                            pro.set_calibration("generic_xrit", calib_cfg);
                            pro.set_channel_unit(pro.images.size() - 1, CALIBRATION_ID_ALBEDO);
                        }
                        else if (lines[2] == "_UNIT:=KELVIN")
                        {
                            std::vector<std::pair<int, float>> lut;
                            for (size_t i = 3; i < lines.size(); i++)
                            {
                                int val;
                                float valo;
                                if (sscanf(lines[i].c_str(), "%d:=%f", &val, &valo) == 2)
                                    lut.push_back({val, valo});
                            }

                            nlohmann::json calib_cfg = pro.get_calibration_raw();

                            calib_cfg["bits_for_calib"] = bits_for_calib;

                            calib_cfg[channel] = lut;
                            pro.set_calibration("generic_xrit", calib_cfg);
                            pro.set_channel_unit(pro.images.size() - 1, CALIBRATION_ID_EMISSIVE_RADIANCE);
                        }
                    }

                    // Himawari xRIT modes
                    if (instrument_id == "ahi")
                    {
                        int bits_for_calib = 8;
                        if (lines[0].find("HALFTONE:=10") != std::string::npos)
                            bits_for_calib = 10;
                        if (lines[0].find("HALFTONE:=16") != std::string::npos)
                            bits_for_calib = 10; // Yes, they send 10 over 16
                        if (lines[2].find("_UNIT:=PERCENT") != std::string::npos || lines[2].find("_UNIT:=ALBEDO(%)") != std::string::npos)
                        {
                            std::vector<std::pair<int, float>> lut;
                            for (size_t i = 3; i < lines.size(); i++)
                            {
                                int val;
                                float valo;
                                if (sscanf(lines[i].c_str(), "%d:=%f", &val, &valo) == 2)
                                    lut.push_back({val, valo / 100.0});
                            }

                            nlohmann::json calib_cfg = pro.get_calibration_raw();

                            calib_cfg["bits_for_calib"] = bits_for_calib;
                            calib_cfg["to_complete"] = true;

                            calib_cfg[channel] = lut;
                            pro.set_calibration("generic_xrit", calib_cfg);
                            pro.set_channel_unit(pro.images.size() - 1, CALIBRATION_ID_ALBEDO);
                        }
                        else if (lines[2].find("_UNIT:=KELVIN") != std::string::npos)
                        {
                            std::vector<std::pair<int, float>> lut;
                            for (size_t i = 3; i < lines.size(); i++)
                            {
                                int val;
                                float valo;
                                if (sscanf(lines[i].c_str(), "%d:=%f", &val, &valo) == 2)
                                    lut.push_back({val, valo});
                            }

                            nlohmann::json calib_cfg = pro.get_calibration_raw();

                            calib_cfg["bits_for_calib"] = bits_for_calib;
                            calib_cfg["to_complete"] = true;

                            calib_cfg[channel] = lut;
                            pro.set_calibration("generic_xrit", calib_cfg);
                            pro.set_channel_unit(pro.images.size() - 1, CALIBRATION_ID_EMISSIVE_RADIANCE);
                        }
                    }
                }
            }
        }

        void XRITChannelProcessor::saveImg(xrit::XRITFileInfo &meta, image::Image &img)
        {
            // If we lack some essential metadata, we have to exit
            if (meta.channel == "" || meta.satellite_name == "" || meta.satellite_short_name == "" || meta.timestamp == -1)
            {
                if (in_memory_mode)
                {
                    logger->error("TODOREWORK Support in-memory raw images!");
                    return;
                }

                // Make sure directories exist
                if (!std::filesystem::exists(directory + "/IMAGES/Unknown/"))
                    std::filesystem::create_directories(directory + "/IMAGES/Unknown/");
                image::save_img(img, std::string(directory + "/IMAGES/Unknown/" + meta.filename).c_str());
                return;
            }

            // GOES-specific stuff TODOREWORKXRIT
            if (meta.type == XRIT_GOES_ABI)
            {
                // TODO: Once calibration of custom types is possible, stop doing this!
                // RRPQE inverts its raw counts and lookup tables seemingly randomly.
                // So far, only RRQPE seems to do this...
                if (meta.channel == "RRQPE" && img.get(0) == 255)
                    image::linear_invert(img);
            }

            // Utils
            const auto &region = meta.region;
            const auto &satellite = meta.satellite_name;
            const auto &timestamp = meta.timestamp;
            const auto &satshort = meta.satellite_short_name;
            const auto &channel = meta.channel;
            const auto &bit_depth = meta.bit_depth;
            const auto &instrument_id = meta.instrument_id;

            // Prepare some paths
            std::string ext;
            image::append_ext(&ext, true);
            std::string directory_path =
                region == "" ? (directory + "/" + satellite + "/" + timestamp_to_string2(timestamp) + "/") : (directory + "/" + satellite + "/" + region + "/" + timestamp_to_string2(timestamp) + "/");
            std::string filename = satshort + "_" + channel + "_" + getXRITTimestamp(timestamp) + ext;
            std::string pro_f_path = directory_path + "product.cbor";

            // Make sure directories exist
            if (!in_memory_mode && !std::filesystem::exists(directory_path))
                std::filesystem::create_directories(directory_path);

            // Parse navigation info
            nlohmann::json proj_cfg;
            if (meta.image_navigation_record)
            {
                auto &image_navigation_record = meta.image_navigation_record;

                float sat_pos = 0;
                if (sscanf(image_navigation_record->projection_name.c_str(), "geos(%f)", &sat_pos) == 1 || sscanf(image_navigation_record->projection_name.c_str(), "GEOS(%f)", &sat_pos) == 1)
                {
                    constexpr double k = 624597.0334223134;
                    double scalar_x =
                        image_navigation_record->column_scalar == 0.0 ? (pow(2.0, 16.0) / double(image_navigation_record->column_scaling_factor)) * k : image_navigation_record->column_scalar;
                    double scalar_y = image_navigation_record->line_scalar == 0.0 ? (pow(2.0, 16.0) / double(image_navigation_record->line_scaling_factor)) * k : image_navigation_record->line_scalar;

                    proj_cfg["type"] = "geos";
                    proj_cfg["lon0"] = sat_pos;
                    if (meta.type == XRIT_ELEKTRO_MSUGS || meta.type == XRIT_MSG_SEVIRI)
                        proj_cfg["sweep_x"] = false; // TODOREWORK per instrument in generic function;
                    else if (meta.type == XRIT_GOES_ABI || meta.type == XRIT_GOES_HIMAWARI_AHI || meta.type == XRIT_GOESN_IMAGER)
                        proj_cfg["sweep_x"] = true;
                    else if (meta.type == XRIT_GK2A_AMI)
                        proj_cfg["sweep_x"] = false;
                    else if (meta.type == XRIT_HIMAWARI_AHI)
                        proj_cfg["sweep_x"] = true;
                    else
                        proj_cfg["sweep_x"] = false;
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
            if (!in_memory_mode && std::filesystem::exists(directory_path + filename))
            {
                if (in_memory_mode)
                {
                    logger->error("TODOREWORK Support in-memory raw images!");
                    return;
                }

                int current_iteration = 1;
                std::string filename_new;
                do
                {
                    filename_new = satshort + "_" + channel + "_" + getXRITTimestamp(timestamp) + "_" + std::to_string(current_iteration++) + ext;
                } while (std::filesystem::exists(directory_path + filename_new));
                image::save_img(img, directory_path + filename_new);
                logger->warn("Image already existed. Written as %s", filename_new.c_str());
            }
            // Otherwise, we can go on as usual and write a proper product.
            else
            {
                // To forward to composites if we actually produced products
                std::shared_ptr<products::ImageProduct> pro = nullptr;

                // If the product file already exists, we want to append the new
                // channel to it, without overwriting anything.
                // Products are loaded without loading images.
                if (in_memory_mode ? in_memory_products.count(pro_f_path) : std::filesystem::exists(pro_f_path))
                {
                    if (in_memory_mode)
                    {
                        pro = in_memory_products[pro_f_path];
                    }
                    else
                    {
                        pro = std::make_shared<products::ImageProduct>();
                        pro->d_no_not_save_images = true;
                        pro->d_no_not_load_images = true;
                        pro->load(pro_f_path);
                    }

                    bool contains = false;
                    for (auto &img : pro->images)
                        if (img.channel_name == channel)
                            contains = true;

                    if (!contains)
                        pro->images.push_back({(int)pro->images.size(), filename, channel, in_memory_mode ? img : image::Image(), bit_depth});
                    else
                        logger->error("Image was already contained in product! This should not happen!");
                    pro->contents["xrit_projs"][channel] = proj_cfg; // TODOREWORKXRIT

                    // TODOREWORKXRIT HRV is a pita
                    if (meta.type == XRIT_MSG_SEVIRI && meta.channel == "12")
                        proj_cfg.clear();

                    // TODOREWORKXRIT recheck this logic!
                    if (proj_cfg.size() > 0)
                    {
                        if (!pro->has_proj_cfg())
                        {
                            pro->set_proj_cfg(proj_cfg);
                            // logger->critical("\n%s\n", proj_cfg.dump(4).c_str());
                        }
                        else
                        {
                            if (pro->get_proj_cfg(-1).contains("width") && proj_cfg.contains("width"))
                            {
                                if (proj_cfg["width"].get<int>() > pro->get_proj_cfg(-1)["width"].get<int>())
                                    pro->set_proj_cfg(proj_cfg);
                            }

                            double final_width = pro->get_proj_cfg(-1)["width"].get<int>();
                            // logger->critical(final_width);

                            // Update scales. TODOREWORK the weird case of HRV
                            for (auto &h : pro->images)
                            {
                                auto &p = pro->contents["xrit_projs"][h.channel_name];
                                if (!p.is_null())
                                {
                                    double scale = final_width / p["width"].get<double>();
                                    h.ch_transform = ChannelTransform().init_affine(scale, scale, 0, 0);
                                }
                            }
                        }
                    }
                }
                // Otherwise, create a new product with the current single channel we have.
                else
                {
                    if (in_memory_mode)
                    {
                        pro = std::make_shared<satdump::products::ImageProduct>();
                        in_memory_products.emplace(pro_f_path, pro);
                    }
                    else
                    {
                        pro = std::make_shared<satdump::products::ImageProduct>();
                        pro->d_no_not_save_images = true;
                    }
                    pro->instrument_name = instrument_id;
                    pro->set_product_source(satellite);
                    pro->set_product_timestamp(timestamp);
                    if (proj_cfg.size() > 0)
                    {
                        pro->set_proj_cfg(proj_cfg);
                        // logger->critical("\n%s\n", proj_cfg.dump(4).c_str());
                    }
                    pro->images.push_back({(int)pro->images.size(), filename, channel, in_memory_mode ? img : image::Image(), bit_depth});
                    pro->contents["xrit_projs"][channel] = proj_cfg; // TODOREWORKXRIT
                }

                if (pro)
                {
                    addCalibrationInfoFunc(*pro, meta.image_data_function_record.get(), channel, instrument_id);
                    if (!in_memory_mode)
                        pro->save(directory_path);
                }

                // Save the actual img
                if (in_memory_mode) {}
                else
                {
                    image::save_img(img, directory_path + filename);
                }
            }
        }
    } // namespace xrit
} // namespace satdump