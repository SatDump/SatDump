#include "image_products.h"
#include "logger.h"
#include "core/config.h"
#include "common/image/composite.h"
#include "resources.h"
#include "common/image/earth_curvature.h"
#include <filesystem>
#include "libs/sol2/sol.hpp"
#include "common/lua/lua_utils.h"
#include "common/calibration.h"
#include "core/plugin.h"

namespace satdump
{
    void ImageProducts::save(std::string directory)
    {
        type = "image";
        contents["has_timestamps"] = has_timestamps;
        if (has_timestamps)
            contents["timestamps_type"] = timestamp_type;
        contents["bit_depth"] = bit_depth;
        contents["needs_correlation"] = needs_correlation;

        if (save_as_matrix)
            contents["save_as_matrix"] = save_as_matrix;

        std::string image_format;
        try
        {
            image_format = satdump::config::main_cfg["satdump_general"]["product_format"]["value"];
        }
        catch (std::exception &e)
        {
            logger->error("Product format not specified, using PNG! %s", e.what());
            image_format = "png";
        }

        std::mutex savemtx;
#pragma omp parallel for
        for (size_t c = 0; c < images.size(); c++)
        {
            savemtx.lock();
            if (images[c].filename.find(".png") == std::string::npos &&
                images[c].filename.find(".jpeg") == std::string::npos &&
                images[c].filename.find(".jpg") == std::string::npos &&
                images[c].filename.find(".j2k") == std::string::npos &&
                images[c].filename.find(".pbm") == std::string::npos)
                images[c].filename += "." + image_format;
            else
                logger->trace("Image format was specified in product call. Not supposed to happen!");

            contents["images"][c]["file"] = images[c].filename;
            contents["images"][c]["name"] = images[c].channel_name;

            if (images[c].timestamps.size() > 0)
            {
                contents["images"][c]["timestamps"] = images[c].timestamps;
            }

            contents["images"][c]["ifov_y"] = images[c].ifov_y;
            contents["images"][c]["ifov_x"] = images[c].ifov_x;
            if (images[c].offset_x != 0)
                contents["images"][c]["offset_x"] = images[c].offset_x;

            savemtx.unlock();
            if (!save_as_matrix)
                images[c].image.save_img(directory + "/" + images[c].filename);
        }

        if (save_as_matrix)
        {
            int size = ceil(sqrt(images.size()));
            logger->debug("Using size %d", size);
            image::Image<uint16_t> image_all = image::make_manyimg_composite<uint16_t>(size, size, images.size(), [this](int c)
                                                                                       { return images[c].image; });
            image_all.save_img(directory + "/" + images[0].filename);
            savemtx.lock();
            contents["img_matrix_size"] = size;
            savemtx.unlock();
        }

        Products::save(directory);
    }

    void ImageProducts::load(std::string file)
    {
        Products::load(file);
        std::string directory = std::filesystem::path(file).parent_path().string();

        has_timestamps = contents["has_timestamps"].get<bool>();
        if (has_timestamps)
            timestamp_type = (Timestamp_Type)contents["timestamps_type"].get<int>();
        bit_depth = contents["bit_depth"].get<int>();
        needs_correlation = contents["needs_correlation"].get<bool>();

        if (contents.contains("save_as_matrix"))
            save_as_matrix = contents["save_as_matrix"].get<bool>();

        image::Image<uint16_t> img_matrix;
        if (save_as_matrix)
        {
            if (std::filesystem::exists(directory + "/" + contents["images"][0]["file"].get<std::string>()))
                img_matrix.load_img(directory + "/" + contents["images"][0]["file"].get<std::string>());
        }

        for (size_t c = 0; c < contents["images"].size(); c++)
        {
            ImageHolder img_holder;
            if (!save_as_matrix)
                logger->info("Loading " + contents["images"][c]["file"].get<std::string>());

            img_holder.filename = contents["images"][c]["file"].get<std::string>();
            img_holder.channel_name = contents["images"][c]["name"].get<std::string>();

            if (!save_as_matrix)
            {
                if (std::filesystem::exists(directory + "/" + contents["images"][c]["file"].get<std::string>()))
                    img_holder.image.load_img(directory + "/" + contents["images"][c]["file"].get<std::string>());
            }
            else
            {
                int m_size = contents["img_matrix_size"].get<int>();
                int img_width = img_matrix.width() / m_size;
                int img_height = img_matrix.height() / m_size;
                int pos_x = c % m_size;
                int pos_y = c / m_size;

                int px_pos_x = pos_x * img_width;
                int px_pos_y = pos_y * img_height;

                img_holder.image = img_matrix.crop_to(px_pos_x, px_pos_y, px_pos_x + img_width, px_pos_y + img_height);
            }

            if (contents["images"][c].contains("timestamps"))
            {
                img_holder.timestamps = contents["images"][c]["timestamps"].get<std::vector<double>>();
            }

            img_holder.ifov_y = contents["images"][c]["ifov_y"].get<int>();
            img_holder.ifov_x = contents["images"][c]["ifov_x"].get<int>();
            if (contents["images"][c].contains("offset_x"))
                img_holder.offset_x = contents["images"][c]["offset_x"].get<int>();

            images.push_back(img_holder);
        }
    }

    void ImageProducts::init_calibration()
    {
        calib_mutex.lock();
        if (contents["calibration"].contains("lua"))
        { // Lua-based calibration
            if (lua_state_ptr == nullptr)
            {
                lua_state_ptr = new sol::state();
                sol::state &lua = *((sol::state *)lua_state_ptr);

                lua.open_libraries(sol::lib::base);
                lua.open_libraries(sol::lib::string);
                lua.open_libraries(sol::lib::math);

                lua["wavenumbers"] = contents["calibration"]["wavenumbers"].get<std::vector<double>>();
                lua["lua_vars"] = lua_utils::mapJsonToLua(lua, contents["calibration"]["lua_vars"]);

                std::string lua_code = contents["calibration"]["lua"].get<std::string>();
                lua.script(lua_code);

                lua["init"].call();

                lua_comp_func_ptr = new sol::function();
                *((sol::function *)lua_comp_func_ptr) = lua["compute"];
            }
        }
        else if (contents["calibration"].contains("calibrator"))
        { // Callback-based calibration
            std::string calibrator_id = contents["calibration"]["calibrator"].get<std::string>();

            std::vector<std::shared_ptr<CalibratorBase>> calibrators;
            satdump::eventBus->fire_event<RequestCalibratorEvent>({calibrator_id, calibrators, contents["calibration"], this});
            if (calibrators.size() > 0)
                calibrator_ptr = calibrators[0];
            else
                logger->error("Requested calibrator " + calibrator_id + " does not exist!");

            calibrator_ptr->init();
        }
        calib_mutex.unlock();
    }

    ImageProducts::~ImageProducts()
    {
        if (lua_state_ptr != nullptr)
        {
            delete (sol::function *)lua_comp_func_ptr;
            delete (sol::state *)lua_state_ptr;
        }
        if (calibrator_ptr != nullptr)
        {
            calibrator_ptr.reset();
        }
    }

    double ImageProducts::get_calibrated_value(int image_index, int x, int y, bool temp)
    {
        calib_mutex.lock();
        uint16_t val = images[image_index].image[y * images[image_index].image.width() + x] >> (16 - bit_depth);
        double val2 = CALIBRATION_INVALID_VALUE;
        if (calibrator_ptr != nullptr)
            val2 = calibrator_ptr->compute(image_index, x, y, val);
        else if (lua_state_ptr != nullptr)
            val2 = ((sol::function *)lua_comp_func_ptr)->call(image_index, x, y, val).get<double>();
        if (get_calibration_type(image_index) == calib_type_t::CALIB_RADIANCE && temp)
            val2 = radiance_to_temperature(val2, get_wavenumber(image_index));
        calib_mutex.unlock();
        return val2;
    }

    image::Image<uint16_t> ImageProducts::get_calibrated_image(int image_index, float *progress, calib_vtype_t vtype, std::pair<double, double> range)
    {
        bool is_default = vtype == CALIB_VTYPE_AUTO && range.first == 0 && range.second == 0;
        if (calibrated_img_cache.count(image_index) > 0 && is_default)
        {
            logger->trace("Cached calibrated image channel %d", image_index + 1);
            return calibrated_img_cache[image_index];
        }
        else
        {
            double wn = get_wavenumber(image_index);

            if (range.first == 0 && range.second == 0)
            {
                range = get_calibration_default_radiance_range(image_index);

                if (get_calibration_type(image_index) == CALIB_RADIANCE)
                {
                    if (vtype == CALIB_VTYPE_TEMPERATURE)
                        range = {radiance_to_temperature(range.first, wn),
                                 radiance_to_temperature(range.second, wn)};
                }
            }

            logger->trace("Generating calibrated image channel %d. Range %f %f. Type %d", image_index + 1, range.first, range.second, vtype);

            // calibrated_img_cache.insert({image_index, image::Image<uint16_t>(images[image_index].image.width(), images[image_index].image.height(), 1)});
            // image::Image<uint16_t> &output = calibrated_img_cache[image_index];
            image::Image<uint16_t> output(images[image_index].image.width(), images[image_index].image.height(), 1);

            if (vtype == CALIB_VTYPE_AUTO && get_calibration_type(image_index) == CALIB_RADIANCE)
                vtype = CALIB_VTYPE_RADIANCE;
            else if (vtype == CALIB_VTYPE_AUTO && get_calibration_type(image_index) == CALIB_REFLECTANCE)
                vtype = CALIB_VTYPE_ALBEDO;

            try
            {
                for (size_t x = 0; x < images[image_index].image.width(); x++)
                {
                    for (size_t y = 0; y < images[image_index].image.height(); y++)
                    {
                        double cal_val = get_calibrated_value(image_index, x, y);

                        if (vtype == CALIB_VTYPE_TEMPERATURE && get_calibration_type(image_index) == CALIB_RADIANCE)
                            cal_val = radiance_to_temperature(cal_val, wn);

                        output[y * output.width() + x] =
                            output.clamp(
                                ((cal_val - range.first) /
                                 abs(range.first - range.second)) *
                                65535);
                    }

                    if (progress != nullptr)
                        *progress = float(x) / float(images[image_index].image.width());
                }
            }
            catch (std::exception &e)
            {
                logger->error("Error calibrating image : %s", e.what());
            }

            if (is_default)
                calibrated_img_cache.insert({image_index, output});

            return output;
        }
    }

    bool equation_contains(std::string init, std::string match)
    {
        size_t pos = init.find(match);
    retry:
        if (pos != std::string::npos)
        {
            std::string final_ex;
            while (pos < init.size())
            {
                char v = init[pos];
                if (v < 48 || v > 57)
                    if (v < 65 || v > 90)
                        if (v < 97 || v > 122)
                            break;
                final_ex += v;
                pos++;
            }

            if (match == final_ex)
                return true;

            pos = init.find(match, pos + 1);
            goto retry;
        }

        return false;
    }

    void get_calib_cfg_from_json(nlohmann::json v, ImageProducts::calib_vtype_t &type, std::pair<double, double> &range)
    {
        std::string vtype = v["type"].get<std::string>();
        range.first = v["min"].get<double>();
        range.second = v["max"].get<double>();
        if (vtype == "auto")
            type = ImageProducts::CALIB_VTYPE_AUTO;
        else if (vtype == "albedo")
            type = ImageProducts::CALIB_VTYPE_ALBEDO;
        else if (vtype == "radiance")
            type = ImageProducts::CALIB_VTYPE_RADIANCE;
        else if (vtype == "temperature")
            type = ImageProducts::CALIB_VTYPE_TEMPERATURE;
    }

    image::Image<uint16_t> make_composite_from_product(ImageProducts &product, ImageCompositeCfg cfg, float *progress, std::vector<double> *final_timestamps, nlohmann::json *final_metadata)
    {
        std::vector<int> channel_indexes;
        std::vector<std::string> channel_numbers;
        std::vector<image::Image<uint16_t>> images_obj;
        std::map<std::string, int> offsets;

        int max_width_total = 0;
        int max_width_used = 1;
        int min_offset = 100000000;

        if (final_metadata != nullptr)
            final_metadata->clear();

        std::string str_to_find_channels = cfg.equation;

        if (cfg.lut.size() != 0 || cfg.lua.size() != 0)
            str_to_find_channels = cfg.channels;

        for (int i = 0; i < (int)product.images.size(); i++)
        {
            auto img = product.images[i];
            std::string equ_str = "ch" + img.channel_name;
            std::string equ_str_calib = "cch" + img.channel_name;

            if (max_width_total < (int)img.image.width())
                max_width_total = img.image.width();

            if (equation_contains(str_to_find_channels, equ_str) && img.image.size() > 0)
            {
                channel_indexes.push_back(i);
                channel_numbers.push_back(equ_str);
                images_obj.push_back(img.image);
                offsets.emplace(equ_str, img.offset_x);
                logger->debug("Composite needs channel %s", equ_str.c_str());

                if (max_width_used < (int)img.image.width())
                    max_width_used = img.image.width();

                if (min_offset > img.offset_x)
                    min_offset = img.offset_x;
            }

            if (equation_contains(str_to_find_channels, equ_str_calib) && img.image.size() > 0 && product.has_calibation())
            {
                product.init_calibration();
                channel_indexes.push_back(i);
                channel_numbers.push_back(equ_str_calib);
                if (cfg.calib_cfg.contains(equ_str_calib))
                {
                    ImageProducts::calib_vtype_t type;
                    std::pair<double, double> range;
                    get_calib_cfg_from_json(cfg.calib_cfg[equ_str_calib], type, range);
                    images_obj.push_back(product.get_calibrated_image(i, progress, type, range));
                }
                else
                {
                    images_obj.push_back(product.get_calibrated_image(i, progress));
                }
                offsets.emplace(equ_str_calib, img.offset_x);
                logger->debug("Composite needs calibrated channel %s", equ_str.c_str());

                if (max_width_used < (int)img.image.width())
                    max_width_used = img.image.width();

                if (min_offset > img.offset_x)
                    min_offset = img.offset_x;
            }
        }

        if (channel_indexes.size() == 0)
        {
            logger->error("None of the required channels are present!");
            return image::Image<uint16_t>();
        }

        int ratio = max_width_total / max_width_used;
        logger->trace("Max Total Width %d", max_width_total);
        logger->trace("Max Total Used  %d", max_width_used);

        // Offset... Offsets to 0 and scale if needed
        for (std::pair<const std::string, int> &img_off : offsets)
        {
            img_off.second -= min_offset;
            img_off.second /= ratio;
            logger->trace("Offset for %s is %d", img_off.first.c_str(), img_off.second);

            if (final_metadata != nullptr)
                (*final_metadata)["img_x_offset"] = min_offset;
        }

        if (product.needs_correlation)
        {
            std::vector<double> common_timestamps; // First, establish common timestamps between all required channels
            if (product.timestamp_type == satdump::ImageProducts::Timestamp_Type::TIMESTAMP_MULTIPLE_LINES ||
                product.timestamp_type == satdump::ImageProducts::Timestamp_Type::TIMESTAMP_LINE)
            {
                bool single_line = product.timestamp_type == satdump::ImageProducts::Timestamp_Type::TIMESTAMP_LINE;

                for (double time1 : product.get_timestamps(channel_indexes[0]))
                {
                    bool is_present_everywhere = true;
                    for (int i = 1; i < (int)channel_indexes.size(); i++)
                    {
                        bool contains = false;
                        for (double time2 : product.get_timestamps(channel_indexes[i]))
                        {
                            if (time1 == time2)
                            {
                                contains = true;
                                break;
                            }
                        }

                        if (!contains)
                        {
                            is_present_everywhere = false;
                            break;
                        }
                    }

                    if (is_present_everywhere)
                        common_timestamps.push_back(time1);
                }

                // Now we know all timestamps that *are* common, so we can create new images and copy over
                std::vector<image::Image<uint16_t>> images_obj_new;
                for (int i = 0; i < (int)channel_indexes.size(); i++)
                    images_obj_new.push_back(image::Image<uint16_t>(product.images[channel_indexes[i]].image.width(),
                                                                    (single_line ? 1 : product.get_ifov_y_size(channel_indexes[i])) * common_timestamps.size(),
                                                                    1));

                // Recompose images to be synced
                int y_index = 0;
                for (double time1 : common_timestamps)
                {
                    for (int i = 0; i < (int)channel_indexes.size(); i++)
                    {
                        int index = channel_indexes[i];
                        for (int t = 0; t < (int)product.get_timestamps(index).size(); t++)
                        {
                            double time2 = product.get_timestamps(index)[t];

                            if (time1 == time2)
                            {
                                //  Copy over scanlines
                                memcpy(&images_obj_new[i][y_index * images_obj_new[i].width() * (single_line ? 1 : product.get_ifov_y_size(index))],
                                       &product.images[index].image[t * images_obj_new[i].width() * (single_line ? 1 : product.get_ifov_y_size(index))],
                                       images_obj_new[i].width() * (single_line ? 1 : product.get_ifov_y_size(index)) * sizeof(uint16_t));
                                break;
                            }
                        }
                    }

                    y_index++;
                }

                images_obj = images_obj_new;
            }

            if (final_timestamps != nullptr)
                *final_timestamps = common_timestamps;
        }
        else
        {
            if (final_timestamps != nullptr)
                *final_timestamps = product.get_timestamps();
        }

        image::Image<uint16_t> rgb_composite;

        if (cfg.lua != "")
            rgb_composite = image::generate_composite_from_lua(&product, images_obj, channel_numbers, resources::getResourcePath(cfg.lua), cfg.lua_vars, offsets, progress);
        else if (cfg.lut != "")
            rgb_composite = image::generate_composite_from_lut(images_obj, channel_numbers, resources::getResourcePath(cfg.lut), offsets, progress);
        else
            rgb_composite = image::generate_composite_from_equ(images_obj, channel_numbers, cfg.equation, offsets, progress);

        if (cfg.despeckle)
            rgb_composite.kuwahara_filter();

        if (cfg.equalize)
            rgb_composite.equalize();

        if (cfg.individual_equalize)
            rgb_composite.equalize(true);

        if (cfg.white_balance)
            rgb_composite.white_balance();

        if (cfg.invert)
            rgb_composite.linear_invert();

        if (cfg.normalize)
            rgb_composite.normalize();

        if (cfg.apply_lut)
        {
            auto lut_image = image::LUT_jet<uint16_t>();
            rgb_composite.to_rgb();
            for (size_t i = 0; i < rgb_composite.width() * rgb_composite.height(); i++)
            {
                uint16_t val = rgb_composite[i];
                val = (float(val) / 65535.0) * lut_image.width();
                if (val >= lut_image.width())
                    val = lut_image.width() - 1;
                rgb_composite.channel(0)[i] = lut_image.channel(0)[val];
                rgb_composite.channel(1)[i] = lut_image.channel(1)[val];
                rgb_composite.channel(2)[i] = lut_image.channel(2)[val];
            }
        }

        return rgb_composite;
    }

    image::Image<uint16_t> perform_geometric_correction(ImageProducts &product, image::Image<uint16_t> img, bool &success, float *foward_table)
    {
        if (img.width() == 0)
            return img;

        success = false;
        if (!product.contents.contains("projection_cfg"))
            return img;
        if (!product.get_proj_cfg().contains("corr_swath"))
            return img;
        if (!product.get_proj_cfg().contains("corr_resol"))
            return img;
        if (!product.get_proj_cfg().contains("corr_altit"))
            return img;

        float swath = product.get_proj_cfg()["corr_swath"].get<float>();
        float resol = product.get_proj_cfg()["corr_resol"].get<float>();
        float altit = product.get_proj_cfg()["corr_altit"].get<float>();
        success = true;

        resol *= float(product.images[0].image.width()) / float(img.width());

        if (product.get_proj_cfg().contains("corr_width"))
        {
            if ((int)img.width() != product.get_proj_cfg()["corr_width"].get<int>())
            {
                logger->debug("Image width mistmatch %d %d", product.get_proj_cfg()["corr_width"].get<int>(), img.width());
                resol *= product.get_proj_cfg()["corr_width"].get<int>() / float(img.width());
            }
        }

        return image::earth_curvature::correct_earth_curvature(img, altit, swath, resol, foward_table);
    }

    std::vector<int> generate_horizontal_corr_lut(ImageProducts &product, int width)
    {
        if (!product.contents.contains("projection_cfg"))
            return {-1};
        if (!product.get_proj_cfg().contains("corr_swath"))
            return {-1};
        if (!product.get_proj_cfg().contains("corr_resol"))
            return {-1};
        if (!product.get_proj_cfg().contains("corr_altit"))
            return {-1};

        float swath = product.get_proj_cfg()["corr_swath"].get<float>();
        float resol = product.get_proj_cfg()["corr_resol"].get<float>();
        float altit = product.get_proj_cfg()["corr_altit"].get<float>();

        resol *= float(product.images[0].image.width()) / float(width);

        if (product.get_proj_cfg().contains("corr_width"))
        {
            if ((int)width != product.get_proj_cfg()["corr_width"].get<int>())
            {
                logger->debug("Image width mistmatch %d %d", product.get_proj_cfg()["corr_width"].get<int>(), width);
                resol *= product.get_proj_cfg()["corr_width"].get<int>() / float(width);
            }
        }

        float satellite_orbit_radius = 6371.0f + altit;                                                                                              // Compute the satellite's orbit radius
        int corrected_width = round(swath / resol);                                                                                                  // Compute the output image size, or number of samples from the imager
        float satellite_view_angle = swath / 6371.0f;                                                                                                // Compute the satellite's view angle
        float edge_angle = -atanf(6371.0f * sinf(satellite_view_angle / 2) / ((cosf(satellite_view_angle / 2)) * 6371.0f - satellite_orbit_radius)); // Max angle relative to the satellite

        std::vector<int> correction_factors(corrected_width);

        // Generate them
        for (int i = 0; i < corrected_width; i++)
        {
            float angle = ((float(i) / float(corrected_width)) - 0.5f) * satellite_view_angle;                          // Get the satellite's angle
            float satellite_angle = -atanf(6371.0f * sinf(angle) / ((cosf(angle)) * 6371.0f - satellite_orbit_radius)); // Convert to an angle relative to earth
            correction_factors[i] = width * ((satellite_angle / edge_angle + 1.0f) / 2.0f);                             // Convert that to a pixel from the original image
        }
        return correction_factors;
    }
}
