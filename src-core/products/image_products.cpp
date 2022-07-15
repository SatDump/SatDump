#include "image_products.h"
#include "logger.h"
#include "common/image/composite.h"
#include "resources.h"
#include "common/image/earth_curvature.h"

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

        if (has_calibation())
            calibration_polynomial_coefs.resize(contents["images"].size());

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

            // Also load calibration if present
            if (has_calibation())
            {
                if (get_calibration_type(c) == POLYNOMIAL)
                    calibration_polynomial_coefs[c][0] = contents["calibration"][c]["coefs"].get<std::vector<double>>();
                else if (get_calibration_type(c) == POLYNOMIAL_PER_LINE)
                    calibration_polynomial_coefs[c] = contents["calibration"][c]["coefs"].get<std::vector<std::vector<double>>>();
                // CUSTOM NOT IMPLEMENTED YET
            }
        }
    }

    double ImageProducts::get_radiance_value(int image_index, int x, int y)
    {
        std::vector<double> coefs;

        if (has_calibation())
        {
            if (get_calibration_type(image_index) == POLYNOMIAL)
                coefs = calibration_polynomial_coefs[image_index][0];
            else if (get_calibration_type(image_index) == POLYNOMIAL_PER_LINE)
                coefs = calibration_polynomial_coefs[image_index][y];
            else
                return 0;
        }
        else
            return 0;

        double raw_value = images[image_index].image[y * images[image_index].image.width() + x] >> (16 - bit_depth);

        double radiance = 0;
        int level = 0;
        for (double c : coefs)
            radiance += c * powf(raw_value, level++);

        return radiance;
    }

    image::Image<uint16_t> make_composite_from_product(ImageProducts &product, ImageCompositeCfg cfg, float *progress)
    {
        std::vector<int> channel_indexes;
        std::vector<std::string> channel_numbers;
        std::vector<image::Image<uint16_t>> images_obj;
        std::map<std::string, int> offsets;

        int max_width_total = 0;
        int max_width_used = 1;
        int min_offset = 100000000;

        std::string str_to_find_channels = cfg.equation;

        if (cfg.lut != "")
            str_to_find_channels = cfg.lut_channels;

        for (int i = 0; i < (int)product.images.size(); i++)
        {
            auto img = product.images[i];
            std::string equ_str = "ch" + img.channel_name;

            if (max_width_total < (int)img.image.width())
                max_width_total = img.image.width();

            if (str_to_find_channels.find(equ_str) != std::string::npos)
            {
                channel_indexes.push_back(i);
                channel_numbers.push_back(img.channel_name);
                images_obj.push_back(img.image);
                offsets.emplace(img.channel_name, img.offset_x);
                logger->debug("Composite needs channel {:s}", equ_str);

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

        // Offset... Offsets to 0 and scale if needed
        for (std::pair<const std::string, int> &img_off : offsets)
        {
            img_off.second -= min_offset;
            img_off.second /= ratio;
        }

        if (product.needs_correlation)
        {
            if (product.timestamp_type == satdump::ImageProducts::Timestamp_Type::TIMESTAMP_MULTIPLE_LINES)
            {
                std::vector<double> common_timestamps; // First, establish common timestamps between all required channels
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
        }

        image::Image<uint16_t> rgb_composite;

        if (cfg.lut == "")
            rgb_composite = image::generate_composite_from_equ(images_obj, channel_numbers, cfg.equation, offsets, progress);
        else
            rgb_composite = image::generate_composite_from_lut(images_obj, channel_numbers, resources::getResourcePath(cfg.lut), offsets, progress);

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

    image::Image<uint16_t> perform_geometric_correction(ImageProducts &product, image::Image<uint16_t> img, bool &success)
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

        return image::earth_curvature::correct_earth_curvature(img, altit, swath, resol);
    }
}