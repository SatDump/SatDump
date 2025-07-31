#include "core/resources.h"
#include "image/image.h"
#include "image/io.h"
#include "image/processing.h"
#include "logger.h"
#include "nlohmann/json.hpp"
#include "nlohmann/json_utils.h"
#include "products/image_product.h"
#include "utils/string.h"
#include "xrit/elektro/specific_proc.h"
#include "xrit/identify.h"
#include "xrit/msg/specific_proc.h"
#include "xrit/processor/calib.h"
#include "xrit/xrit_file.h"
#include "xrit_channel_processor.h"
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>

#include "../../../official_products_support/new/processors/nat/msg/nat_utils.h"

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

        nlohmann::json getProjCfg(xrit::XRITFileInfo &meta, image::Image &img)
        {
            nlohmann::json proj_cfg;
            if (meta.image_navigation_record)
            {
                auto &image_navigation_record = meta.image_navigation_record;

                float sat_pos = 0;
                if (sscanf(image_navigation_record->projection_name.c_str(), "geos(%f)", &sat_pos) == 1 || sscanf(image_navigation_record->projection_name.c_str(), "GEOS(%f)", &sat_pos) == 1)
                {
                    // MSG is inverted... Re-use .nat projection scheme instead of
                    // the xRIT standard. TODOREWORK make common?
                    if (meta.type == XRIT_MSG_SEVIRI)
                    {
                        if (meta.channel == "12")
                            return proj_cfg;

                        int act_height = img.height() > 3712 ? img.height() / 3 : img.height();
                        proj_cfg["type"] = "geos";
                        proj_cfg["lon0"] = sat_pos;
                        proj_cfg["sweep_x"] = false;
                        proj_cfg["scalar_x"] = 3000.403165817;
                        proj_cfg["scalar_y"] = -3000.403165817;
                        proj_cfg["offset_x"] = -5568748.275756353;
                        proj_cfg["offset_y"] = act_height == 3712 ? /*RSS CASE*/ 5572548.275756 : 5568748.275756353;
                        proj_cfg["width"] = 3712;
                        proj_cfg["height"] = act_height;
                        proj_cfg["altitude"] = 35785831.00;
                    }
                    else
                    { // All the rest is sane(r)
                        constexpr double k = 624597.0334223134;
                        double scalar_x =
                            image_navigation_record->column_scalar == 0.0 ? (pow(2.0, 16.0) / double(image_navigation_record->column_scaling_factor)) * k : image_navigation_record->column_scalar;
                        double scalar_y =
                            image_navigation_record->line_scalar == 0.0 ? (pow(2.0, 16.0) / double(image_navigation_record->line_scaling_factor)) * k : image_navigation_record->line_scalar;

                        proj_cfg["type"] = "geos";
                        proj_cfg["lon0"] = sat_pos;
                        if (meta.type == XRIT_ELEKTRO_MSUGS || meta.type == XRIT_MSG_SEVIRI)
                            proj_cfg["sweep_x"] = false;
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
            }
            else if (meta.image_navigation_record_fy4)
            {
                logger->critical("TODOREWORK FY-4x projection!");
#if 0
                auto &image_navigation_record = meta.image_navigation_record_fy4;

                float sat_pos = 0;

                constexpr double k = 624597.0334223134;
                double scalar_x = image_navigation_record->column_scaling_factor; // image_navigation_record->column_scalar == 0.0 ? (pow(2.0, 16.0) /
                                                                                  // double(image_navigation_record->column_scaling_factor)) * k : image_navigation_record->column_scalar;
                double scalar_y = image_navigation_record->line_scaling_factor; // image_navigation_record->line_scalar == 0.0 ? (pow(2.0, 16.0) / double(image_navigation_record->line_scaling_factor))
                                                                                // * k : image_navigation_record->line_scalar;

                logger->critical("%f %f %f %f", image_navigation_record->column_scaling_factor, image_navigation_record->line_scaling_factor, image_navigation_record->line_offset,
                                 image_navigation_record->column_offset);

                proj_cfg["type"] = "geos";
                proj_cfg["lon0"] = sat_pos;
                proj_cfg["sweep_x"] = false;
                proj_cfg["scalar_x"] = scalar_x;
                proj_cfg["scalar_y"] = -scalar_y;
                proj_cfg["offset_x"] = double(image_navigation_record->column_offset) * -scalar_x;
                proj_cfg["offset_y"] = double(image_navigation_record->line_offset) * scalar_y;
                proj_cfg["width"] = img.width();
                proj_cfg["height"] = img.height();
                proj_cfg["altitude"] = 35786023.00;
#endif
            }
            return proj_cfg;
        }

        XRITChannelProcessor::XRITProd XRITChannelProcessor::setupProduct(XRITFileInfo &meta)
        {
            XRITProd p;
            p.directory_path = meta.region == "" ? (directory + "/" + meta.satellite_name + "/" + timestamp_to_string2(meta.timestamp) + "/")
                                                 : (directory + "/" + meta.satellite_name + "/" + meta.region + "/" + timestamp_to_string2(meta.timestamp) + "/");
            p.pro_f_path = p.directory_path + "product.cbor";

            // Make sure directories exist
            if (!in_memory_mode && !std::filesystem::exists(p.directory_path))
                std::filesystem::create_directories(p.directory_path);

            // If the product file already exists, we want to append the new
            // channel to it, without overwriting anything.
            // Products are loaded without loading images.
            if (in_memory_mode ? in_memory_products.count(p.pro_f_path) : std::filesystem::exists(p.pro_f_path))
            {
                if (in_memory_mode)
                {
                    p.pro = in_memory_products[p.pro_f_path];
                }
                else
                {
                    p.pro = std::make_shared<products::ImageProduct>();
                    p.pro->d_no_not_save_images = true;
                    p.pro->d_no_not_load_images = true;
                    p.pro->load(p.pro_f_path);
                }
            }
            // Otherwise, create a new product
            else
            {
                if (in_memory_mode)
                {
                    p.pro = std::make_shared<satdump::products::ImageProduct>();
                    in_memory_products.emplace(p.pro_f_path, p.pro);
                }
                else
                {
                    p.pro = std::make_shared<satdump::products::ImageProduct>();
                    p.pro->d_no_not_save_images = true;
                }
                p.pro->instrument_name = meta.instrument_id;
                p.pro->set_product_source(meta.satellite_name);
                p.pro->set_product_timestamp(meta.timestamp);
            }

            return p;
        }

        bool XRITChannelProcessor::saveMeta(xrit::XRITFileInfo &meta, XRITFile &file)
        {
            // If we lack some essential metadata, we have to exit
            if (meta.satellite_name == "" || meta.satellite_short_name == "" || meta.timestamp == -1)
            {
                logger->error("Got invalid meta file!");
                return false;
            }

            // Get product
            auto p = setupProduct(meta);

            if (meta.type == XRIT_MSG_SEVIRI)
            {
                if (meta.filename.find("PRO") != std::string::npos)
                {
                    if ((file.lrit_data.size() - file.total_header_length) < 425461)
                        return false;
                    logger->info("Parsing SEVIRI Calibration!");

                    uint8_t *header_ptr = &file.lrit_data[file.total_header_length];
                    int hoffset = 60134 + 700 + 326058 + 101 + 72;

                    nlohmann::json sev_config = loadJsonFile(resources::getResourcePath("calibration/SEVIRI_table.json"));
                    nlohmann::json calib_cfg;
                    for (int i = 0; i < 12; i++)
                    {
                        double slope = official::get_r8(&header_ptr[hoffset + (i * 2 + 0) * 8]);
                        double offset = official::get_r8(&header_ptr[hoffset + (i * 2 + 1) * 8]);
                        logger->trace("Channel %d Calibration Slope %f Offset %f", i + 1, slope, offset);
                        calib_cfg["vars"]["slope"][i] = slope;
                        calib_cfg["vars"]["offset"][i] = offset;
                    }
                    calib_cfg["vars"]["buffer"] = sev_config;

                    p.pro->set_calibration("msg_nat_seviri", calib_cfg);
                    tryApplyMSGCalib(p.pro);
                }

                if (meta.filename.find("EPI") != std::string::npos)
                {
                    uint8_t *trailer_ptr = &file.lrit_data[file.total_header_length];

                    int offset = 1 + 2 + 14 + 12 + 192 + 6 * 12 + 16;

                    int LowerEastColumnActual = official::get_i4(&trailer_ptr[offset + 8]);
                    int UpperSouthLineActual = official::get_i4(&trailer_ptr[offset + 16]);
                    int UpperEastColumnActual = official::get_i4(&trailer_ptr[offset + 24]);

                    logger->debug("int LowerEastColumnActual = %d; int UpperSouthLineActual = %d; int UpperEastColumnActual = %d;", LowerEastColumnActual, UpperSouthLineActual, UpperEastColumnActual);

                    p.pro->contents["hrv_buffer"]["LowerEastColumnActual"] = LowerEastColumnActual;
                    p.pro->contents["hrv_buffer"]["UpperSouthLineActual"] = UpperSouthLineActual;
                    p.pro->contents["hrv_buffer"]["UpperEastColumnActual"] = UpperEastColumnActual;

                    // If we already have it, process it now that we can
                    for (auto &h : p.pro->images)
                    {
                        if (h.abs_index == 11)
                        {
                            if (!in_memory_mode) // If needed, load it back up from disk
                                image::load_img(h.image, p.directory_path + "/" + h.filename);
                            tryApplyMSG_HRVProcessing(p.pro, h.image);
                            if (!in_memory_mode)
                                h.image.clear();
                        }
                    }
                }
            }
            else if (meta.type == XRIT_ELEKTRO_MSUGS)
            {
                if (meta.filename.find("PRO") != std::string::npos)
                {
                    if ((file.lrit_data.size() - file.total_header_length) == 612)
                    {
                        logger->info("No MSU-GS calibration in this header!");
                        return true;
                    }
                    if ((file.lrit_data.size() - file.total_header_length) < 41472)
                        return false;
                    logger->info("Parsing MSU-GS Calibration!");

                    uint8_t *header_ptr = &file.lrit_data[file.total_header_length];
                    int hoffset = 612;

                    nlohmann::json msu_config = loadJsonFile(resources::getResourcePath("calibration/MSUGS_table.json"));
                    nlohmann::json calib_cfg;
                    for (int i = 0; i < 10; i++)
                    {
                        for (int y = 0; y < 1024; y++)
                        {
                            int pos = ((i * 1024) + y) * 4;
                            double v = (uint32_t)header_ptr[hoffset + pos + 3] << 24 | //
                                       (uint32_t)header_ptr[hoffset + pos + 2] << 16 | //
                                       (uint32_t)header_ptr[hoffset + pos + 1] << 8 |  //
                                       (uint32_t)header_ptr[hoffset + pos + 0];
                            v /= 1e3;
                            // logger->trace("[%d %d] %f", i, y, v);
                            calib_cfg["vars"]["lut"][i][y] = v;
                        }
                    }
                    calib_cfg["vars"]["buffer"] = msu_config;

                    p.pro->set_calibration("msugs_xrit", calib_cfg);
                    tryApplyELEKTROCalib(p.pro);
                }
            }
            else
            {
                return false;
            }

            if (p.pro && !in_memory_mode)
                p.pro->save(p.directory_path);

            return true;
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

            // Get product
            auto p = setupProduct(meta);

            // Prepare some paths
            std::string ext;
            image::append_ext(&ext, true);
            std::string directory_path = p.directory_path;
            std::string filename = meta.satellite_short_name + "_" + meta.channel + "_" + getXRITTimestamp(meta.timestamp) + ext;
            std::string pro_f_path = directory_path + "product.cbor";
            logger->trace("Saving " + filename + "...");

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
                    filename_new = meta.satellite_short_name + "_" + meta.channel + "_" + getXRITTimestamp(meta.timestamp) + "_" + std::to_string(current_iteration++) + ext;
                } while (std::filesystem::exists(directory_path + filename_new));
                image::save_img(img, directory_path + filename_new);
                logger->warn("Image already existed. Written as %s", filename_new.c_str());
                return;
            }

            // To forward to composites if we actually produced products
            std::shared_ptr<products::ImageProduct> pro = p.pro;

            // Parse navigation info
            nlohmann::json proj_cfg = getProjCfg(meta, img);

            // GOES-specific stuff TODOREWORKXRIT
            if (meta.type == XRIT_GOES_ABI)
            {
                // TODO: Once calibration of custom types is possible, stop doing this!
                // RRPQE inverts its raw counts and lookup tables seemingly randomly.
                // So far, only RRQPE seems to do this...
                if (meta.channel == "RRQPE" && img.get(0) == 255)
                    image::linear_invert(img);
            }

            // FY4x-specific stuff TODOREWORKXRIT
            if (meta.type == XRIT_FY4_AGRI)
            {
                // FY-4x AGRI images are not scaled properly, so normalize for now
                // until we figure this stuff out better
                image::normalize(img);
            }

            // MSG Specific stuff. It's always inverted, undo that
            if (meta.type == XRIT_MSG_SEVIRI)
            {
                img.mirror(true, true);

                // Temporary name for HRV, to avoid composites triggering
                // This gets un-done when HRV is processed if needed
                if (meta.channel == "12")
                {
                    meta.channel = "12_";
                    tryApplyMSG_HRVProcessing(p.pro, img, &meta.channel);
                }
            }

            // Write data in product
            {
                bool contains = false;
                for (auto &img : pro->images)
                    if (img.channel_name == meta.channel)
                        contains = true;

                // By default, but it can be specific as you see below
                int image_channel_index = pro->images.size();

                try
                {
                    // Calib info is split and always for all channels
                    if (meta.type == XRIT_MSG_SEVIRI || meta.type == XRIT_ELEKTRO_MSUGS)
                        image_channel_index = std::stod(meta.channel) - 1;
                }
                catch (std::exception &e)
                {
                    logger->error("Error setting proper channel ID! %s", e.what());
                }

                if (!contains)
                    pro->images.push_back({image_channel_index, filename, meta.channel, in_memory_mode ? img : image::Image(), meta.bit_depth});
                else
                    logger->error("Image was already contained in product! This should not happen!");
                pro->contents["xrit_projs"][meta.channel] = proj_cfg; // TODOREWORKXRIT

                // Special case for MSG's HRV...
                for (auto &h : pro->images)
                    if (meta.type == XRIT_MSG_SEVIRI && h.abs_index == 11)
                        h.ch_transform = ChannelTransform().init_affine(1. / 3., 1. / 3., 0, 0);

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

                        // Update scales
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

            // Those two have the calib info non-standard
            if (meta.type != XRIT_MSG_SEVIRI && meta.type != XRIT_ELEKTRO_MSUGS)
                addCalibrationInfoFunc(*pro, meta.image_data_function_record.get(), meta.channel, meta.instrument_id);
            else if (meta.type == XRIT_MSG_SEVIRI)
                tryApplyMSGCalib(p.pro);
            else if (meta.type == XRIT_ELEKTRO_MSUGS)
                tryApplyELEKTROCalib(p.pro);

            if (!in_memory_mode)
                pro->save(directory_path);

            // Save the actual img
            if (!in_memory_mode)
                image::save_img(img, directory_path + filename);
        }
    } // namespace xrit
} // namespace satdump