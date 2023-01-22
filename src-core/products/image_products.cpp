#include "image_products.h"
#include "logger.h"
#include "common/image/composite.h"
#include "resources.h"
#include "common/image/earth_curvature.h"
#include <filesystem>
#include "libs/sol2/sol.hpp"
#include "common/lua/lua_utils.h"
#include "common/calibration.h"

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

        for (size_t c = 0; c < images.size(); c++)
        {
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

            if (!save_as_matrix)
            {
                logger->info("Saving " + images[c].filename);
                images[c].image.save_png(directory + "/" + images[c].filename);
            }
        }

        if (save_as_matrix)
        {
            int size = ceil(sqrt(images.size()));
            logger->debug("Using size {:d}", size);
            image::Image<uint16_t> image_all = image::make_manyimg_composite<uint16_t>(size, size, images.size(), [this](int c)
                                                                                       { return images[c].image; });
            logger->info("Saving " + images[0].filename);
            image_all.save_png(directory + "/" + images[0].filename);
            contents["img_matrix_size"] = size;
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
                img_matrix.load_png(directory + "/" + contents["images"][0]["file"].get<std::string>());
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
                    img_holder.image.load_png(directory + "/" + contents["images"][c]["file"].get<std::string>());
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
        lua_mutex.lock();
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
        lua_mutex.unlock();
    }

    ImageProducts::~ImageProducts()
    {
        if (lua_state_ptr != nullptr)
        {
            delete (sol::function *)lua_comp_func_ptr;
            delete (sol::state *)lua_state_ptr;
        }
    }

    double ImageProducts::get_calibrated_value(int image_index, int x, int y)
    {
        lua_mutex.lock();
        uint16_t val = images[image_index].image[y * images[image_index].image.width() + x] >> (16 - bit_depth);
        double val2 = ((sol::function *)lua_comp_func_ptr)->call(image_index, x, y, val).get<double>();
        lua_mutex.unlock();
        return val2;
    }

    image::Image<uint16_t> ImageProducts::get_calibrated_image(int image_index, bool force, std::pair<double, double> range)
    {
        if (calibrated_img_cache.count(image_index) > 0 && !force)
        {
            logger->trace("Cached calibrated image channel {:d}", image_index + 1);
            return calibrated_img_cache[image_index];
        }
        else
        {
            if (range.first == 0 && range.second == 0)
                range = get_calibration_default_radiance_range(image_index);

            logger->trace("Generating calibrated image channel {:d}", image_index + 1);

            calibrated_img_cache.insert({image_index, image::Image<uint16_t>(images[image_index].image.width(), images[image_index].image.height(), 1)});
            image::Image<uint16_t> &output = calibrated_img_cache[image_index];

            try
            {
                for (size_t x = 0; x < images[image_index].image.width(); x++)
                {
                    for (size_t y = 0; y < images[image_index].image.height(); y++)
                    {
                        output[y * output.width() + x] = output.clamp(((get_calibrated_value(image_index, x, y) - range.first) / abs(range.first - range.second)) * 65535);
                    }
                }
            }
            catch (std::exception &e)
            {
                logger->error("Error calibrating image : {:s}", e.what());
            }

            return output;
        }
    }

    image::Image<uint16_t> ImageProducts::get_temperature_image(int image_index, bool force, std::pair<double, double> rad_range)
    {
        std::pair<double, double> temp_range = {radiance_to_temperature(rad_range.first, get_wavenumber(image_index)), radiance_to_temperature(rad_range.second, get_wavenumber(image_index))};
        image::Image<uint16_t> temperature_image = get_calibrated_image(image_index, force, rad_range);
        for (unsigned int i = 0; i < temperature_image.size(); i++)
            temperature_image[i] = temperature_image.clamp(((radiance_to_temperature((double)temperature_image[i] / 65535.0 * abs(rad_range.first - rad_range.second) + rad_range.first, get_wavenumber(image_index)) - temp_range.first) / abs(temp_range.first - temp_range.second)) * 65535);
        return temperature_image;
    }

    bool equation_contains(std::string init, std::string match)
    {
        size_t pos = init.find(match);
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
        }

        return false;
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
                logger->debug("Composite needs channel {:s}", equ_str);

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
                images_obj.push_back(product.get_calibrated_image(i));
                offsets.emplace(equ_str_calib, img.offset_x);
                logger->debug("Composite needs calibrated channel {:s}", equ_str);

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
        logger->trace("Max Total Width {:d}", max_width_total);
        logger->trace("Max Total Used  {:d}", max_width_used);

        // Offset... Offsets to 0 and scale if needed
        for (std::pair<const std::string, int> &img_off : offsets)
        {
            img_off.second -= min_offset;
            img_off.second /= ratio;
            logger->trace("Offset for {:s} is {:d}", img_off.first.c_str(), img_off.second);

            if (final_metadata != nullptr)
                (*final_metadata)["img_x_offset"] = min_offset;
        }

        if (product.needs_correlation)
        {
            std::vector<double> common_timestamps; // First, establish common timestamps between all required channels
            if (product.timestamp_type == satdump::ImageProducts::Timestamp_Type::TIMESTAMP_MULTIPLE_LINES)
            {
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
                                                                    product.get_ifov_y_size(channel_indexes[i]) * common_timestamps.size(), 1));

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
                                memcpy(&images_obj_new[i][y_index * images_obj_new[i].width() * product.get_ifov_y_size(index)],
                                       &product.images[index].image[t * images_obj_new[i].width() * product.get_ifov_y_size(index)],
                                       images_obj_new[i].width() * product.get_ifov_y_size(index) * sizeof(uint16_t));
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

        if (cfg.equalize)
            rgb_composite.equalize();

        if (cfg.white_balance)
            rgb_composite.white_balance();

        if (cfg.invert)
            rgb_composite.linear_invert();

        if (cfg.normalize)
            rgb_composite.normalize();

        return rgb_composite;
    }

    image::Image<uint16_t> perform_geometric_correction(ImageProducts &product, image::Image<uint16_t> img, bool &success, float *foward_table)
    {
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
                logger->debug("Image width mistmatch {:d} {:d}", product.get_proj_cfg()["corr_width"].get<int>(), img.width());
                resol *= product.get_proj_cfg()["corr_width"].get<int>() / float(img.width());
            }
        }

        return image::earth_curvature::correct_earth_curvature(img, altit, swath, resol, foward_table);
    }
}