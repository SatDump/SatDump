#include "lrit_productizer.h"
#include "common/utils.h"
#include "logger.h"
#include "products/image_product.h"
#include "utils/string.h"
#include <filesystem>

// TODOREWORK #include "products/processor/image_processor.h"
#include "core/config.h"
#include "nlohmann/json_utils.h"
#include "utils/thread_priority.h"

#include "image/io.h"

namespace lrit
{
    inline std::string getXRITTimestamp(time_t tim)
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
        return std::to_string(timeReadable->tm_year + 1900) + "-" + (timeReadable->tm_mon + 1 > 9 ? std::to_string(timeReadable->tm_mon + 1) : "0" + std::to_string(timeReadable->tm_mon + 1)) + "-" +
               (timeReadable->tm_mday > 9 ? std::to_string(timeReadable->tm_mday) : "0" + std::to_string(timeReadable->tm_mday)) + "_" +
               (timeReadable->tm_hour > 9 ? std::to_string(timeReadable->tm_hour) : "0" + std::to_string(timeReadable->tm_hour)) + "-" +
               (timeReadable->tm_min > 9 ? std::to_string(timeReadable->tm_min) : "0" + std::to_string(timeReadable->tm_min)) + "-" +
               (timeReadable->tm_sec > 9 ? std::to_string(timeReadable->tm_sec) : "0" + std::to_string(timeReadable->tm_sec));
    }

    LRITProductizer::LRITProductizer(std::string instrument_id, bool sweep_x, std::string cache_path) : should_sweep_x(sweep_x), instrument_id(instrument_id), compo_cache_path(cache_path)
    {
        // if (satdump::config::main_cfg["explorer"]["instruments"].contains(instrument_id) && satdump::config::main_cfg["satdump_general"]["auto_process_products"]["value"].get<bool>())
        // TODOREWORK    can_make_composites = true;

        if (can_make_composites)
            compositeGeneratorThread = std::thread(&LRITProductizer::compositeThreadFunc, this);
    }

    LRITProductizer::~LRITProductizer()
    {
        if (can_make_composites)
        {
            int queue_size;
            do
            {
                compo_queue_mtx.lock();
                queue_size = compo_queue.size();
                compo_queue_mtx.unlock();
                std::this_thread::sleep_for(std::chrono::seconds(1));
            } while (queue_size > 0);

            composite_th_should_run = false;
            if (compositeGeneratorThread.joinable())
                compositeGeneratorThread.join();
        }
    }

    // This will most probably get moved over to each
    inline void addCalibrationInfoFunc(satdump::products::ImageProduct &pro, ImageDataFunctionRecord *image_data_function_record, std::string channel, std::string instrument_id)
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

            auto lines = satdump::splitString(image_data_function_record->datas, '\n');
            if (lines.size() < 4)
            {
                lines = satdump::splitString(image_data_function_record->datas, '\r');
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

    /*inline void attemptToGenerateComposites(satdump::products::ImageProduct *pro, std::string pro_path)
    {
        try
        {
            // Get instrument settings
            nlohmann::ordered_json instrument_explorer_settings;
            if (satdump::config::main_cfg["explorer"]["instruments"].contains(pro->instrument_name))
                instrument_explorer_settings = satdump::config::main_cfg["explorer"]["instruments"][pro->instrument_name];
            else
                logger->error("Unknown instrument : %s!", pro->instrument_name.c_str());

            // Generate composites
            if (instrument_explorer_settings.contains("rgb_composites"))
            {
                bool can_make_composites = false;
                for (nlohmann::detail::iteration_proxy_value<nlohmann::detail::iter_impl<nlohmann::ordered_json>> compo : instrument_explorer_settings["rgb_composites"].items())
                {
                    if (compo.value().contains("autogen"))
                        if (compo.value()["autogen"].get<bool>() == false)
                            continue;
                    if (pro->contents.contains("autocomposite_cache_done"))
                        if (pro->contents["autocomposite_cache_done"].contains(compo.key()))
                            continue;

                    // Check each can be made
                    satdump::ImageCompositeCfg cfg = compo.value();
                    if (satdump::check_composite_from_product_can_be_made(*pro, cfg))
                    {
                        // Load ONLY the images needed for these composites
                        can_make_composites = true;
                        std::string str_to_find_channels = cfg.equation;
                        if (cfg.lut.size() != 0 || cfg.lua.size() != 0 || cfg.cpp.size() != 0)
                            str_to_find_channels = cfg.channels;

                        for (int i = 0; i < (int)pro->images.size(); i++)
                        {
                            auto &img = pro->images[i];
                            std::string equ_str = "ch" + img.channel_name;
                            int loc;

                            if (satdump::image_equation_contains(str_to_find_channels, equ_str, &loc) && img.image.size() == 0)
                            {
                                logger->trace("Loading image channel " + img.channel_name);
                                image::load_img(img.image, pro_path + "/" + img.filename);
                            }
                        }
                    }
                }

                // Generate all composites currently possible
                if (can_make_composites)
                {
                    pro->contents["autocomposite_cache_enabled"] = true;
                    satdump::process_image_products((satdump::Products *)pro, pro_path);
                }
            }
        }
        catch (std::exception &e)
        {
            logger->error("Error trying to autogen xRIT composite! : %s", e.what());
        }

        // Unload everything
        for (int i = 0; i < (int)pro->images.size(); i++)
            pro->images[i].image.clear();
    }*/
    // TODOREWORK THIS MAKE WORK AGAIN!

    void LRITProductizer::compositeThreadFunc()
    {
        satdump::setLowestThreadPriority(); // TODOREWORK namespace remove

        std::string file_for_cache = compo_cache_path + "/.composite_cache_do_not_delete.json";

        while (composite_th_should_run)
        {
            compo_queue_mtx.lock();
            int queue_size = compo_queue.size();
            compo_queue_mtx.unlock();

            if (queue_size > 0)
            {
                compo_queue_mtx.lock();
                satdump::products::ImageProduct *pro = (satdump::products::ImageProduct *)compo_queue[0].first;
                std::string directory_path = compo_queue[0].second;
                compo_queue.erase(compo_queue.begin());
                compo_queue_mtx.unlock();

                std::string directory_path_rel = std::filesystem::relative(directory_path, compo_cache_path).string();

                try
                {
                    if (pro != nullptr)
                    {
                        // Load cache
                        auto filecache = loadJsonFile(file_for_cache);
                        if (filecache.contains(directory_path_rel))
                            pro->contents["autocomposite_cache_done"] = filecache[directory_path_rel]["compos"];

                        // Try to generate compos
                        // TODOREWORK                        attemptToGenerateComposites(pro, directory_path);

                        // Write those we just generated to not redo them more than once...
                        if (pro->contents.contains("autocomposite_cache_done"))
                        {
                            filecache[directory_path_rel]["compos"] = pro->contents["autocomposite_cache_done"];
                            filecache[directory_path_rel]["time"] = time(0);
                        }

                        // Delete very old metadata
                        time_t ctime = time(0);
                        for (auto it = filecache.begin(); it != filecache.end();)
                        {
                            if (ctime - it.value()["time"].get<time_t>() > 3600 * 24)
                                it = filecache.erase(it);
                            else
                                ++it;
                        }

                        if (!filecache.is_null())
                            saveJsonFile(file_for_cache, filecache);

                        delete pro;
                        pro = nullptr;
                    }
                }
                catch (std::exception &e)
                {
                    logger->error("Error trying to autogen composites! %s", e.what());
                    delete pro;
                }
            }
            else
            {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    }

    void LRITProductizer::saveImage(image::Image img, int bit_depth, std::string directory, std::string satellite, std::string satshort, std::string channel, time_t timestamp, std::string region,
                                    lrit::ImageNavigationRecord *image_navigation_record, ImageDataFunctionRecord *image_data_function_record)
    {
        std::string ext;
        image::append_ext(&ext, true);
        std::string directory_path =
            region == "" ? (directory + "/" + satellite + "/" + timestamp_to_string2(timestamp) + "/") : (directory + "/" + satellite + "/" + region + "/" + timestamp_to_string2(timestamp) + "/");
        std::string filename = satshort + "_" + channel + "_" + getXRITTimestamp(timestamp) + ext;
        std::string pro_f_path = directory_path + "product.cbor";

        if (!std::filesystem::exists(directory_path))
            std::filesystem::create_directories(directory_path);

        nlohmann::json proj_cfg;
        if (image_navigation_record)
        {
            float sat_pos = 0;
            if (sscanf(image_navigation_record->projection_name.c_str(), "geos(%f)", &sat_pos) == 1 || sscanf(image_navigation_record->projection_name.c_str(), "GEOS(%f)", &sat_pos) == 1)
            {
                constexpr double k = 624597.0334223134;
                double scalar_x =
                    image_navigation_record->column_scalar == 0.0 ? (pow(2.0, 16.0) / double(image_navigation_record->column_scaling_factor)) * k : image_navigation_record->column_scalar;
                double scalar_y = image_navigation_record->line_scalar == 0.0 ? (pow(2.0, 16.0) / double(image_navigation_record->line_scaling_factor)) * k : image_navigation_record->line_scalar;

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

        // To forward to composites if we actually produced products
        satdump::products::ImageProduct *pro = nullptr;

        // Check if the image already exists, this can happen when GOES goes (laugh please, pretty please) wrong.
        // If it does, we append a number and do NOT overwrite.
        if (std::filesystem::exists(directory_path + filename))
        {
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
            // If the product file already exists, we want to append the new
            // channel to it, without overwriting anything.
            // Products are loaded without loading images.
            if (std::filesystem::exists(pro_f_path))
            {
                pro = new satdump::products::ImageProduct();
                pro->d_no_not_save_images = true;
                pro->d_no_not_load_images = true;
                pro->load(pro_f_path);

                bool contains = false;
                for (auto &img : pro->images)
                    if (img.channel_name == channel)
                        contains = true;

                if (!contains)
                    pro->images.push_back({(int)pro->images.size(), filename, channel, image::Image(), bit_depth}); // TODO check index system?

                if (!pro->has_proj_cfg()) // TODOREWORK recheck this logic!
                {
                    if (proj_cfg.size() > 0)
                    {
                        pro->set_proj_cfg(proj_cfg);
                        // logger->critical("\n%s\n", proj_cfg.dump(4).c_str());
                    }
                }
                else
                {
                    if (pro->get_proj_cfg(0).contains("width") && proj_cfg.contains("width"))
                        if (proj_cfg["width"].get<int>() > pro->get_proj_cfg(0)["width"].get<int>())
                            pro->set_proj_cfg(proj_cfg);
                }

                addCalibrationInfoFunc(*pro, image_data_function_record, channel, instrument_id);

                pro->save(directory_path);
            }
            // Otherwise, create a new product with the current single channel we have.
            else
            {
                pro = new satdump::products::ImageProduct();
                pro->d_no_not_save_images = true;
                pro->instrument_name = instrument_id;
                pro->set_product_source(satellite);
                pro->set_product_timestamp(timestamp);
                if (proj_cfg.size() > 0)
                {
                    pro->set_proj_cfg(proj_cfg);
                    // logger->critical("\n%s\n", proj_cfg.dump(4).c_str());
                }
                pro->images.push_back({(int)pro->images.size(), filename, channel, image::Image(), bit_depth});

                addCalibrationInfoFunc(*pro, image_data_function_record, channel, instrument_id);

                pro->save(directory_path);
            }
        }

        image::save_img(img, directory_path + filename);

        // Attempt to autogen composites
        if (can_make_composites)
        {
            // We do NOT want to overload our queue and run out of RAM...
            {
                int queue_size;
                do
                {
                    compo_queue_mtx.lock();
                    queue_size = compo_queue.size();
                    compo_queue_mtx.unlock();
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                } while (queue_size > 50);
            }

            compo_queue_mtx.lock();
            if (pro != nullptr)
                compo_queue.push_back({pro, directory_path});
            compo_queue_mtx.unlock();
        }
        else
        {
            // if (pro != nullptr)
            // {
            //     attemptToGenerateComposites(pro, directory_path);
            //     pro->save(directory_path); // Re-save CBOR!
            // }

            if (pro != nullptr)
                delete pro;
        }
    }
} // namespace lrit