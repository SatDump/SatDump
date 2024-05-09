#include "image_products.h"
#include "logger.h"
#include "core/config.h"
#include "common/image/composite.h"
#include "common/image/image_background.h"
#include "common/image/earth_curvature.h"
#include "resources.h"
#include <filesystem>
#include "libs/sol2/sol.hpp"
#include "common/lua/lua_utils.h"
#include "common/calibration.h"
#include "core/plugin.h"
#include "common/utils.h"
#include "common/image/brightness_contrast.h"
#include "common/image/image_processing.h"
#include "common/image/io.h"
#include "common/image/image_utils.h"
#include "common/image/image_lut.h"

#ifdef __ANDROID__
#include <android_native_app_glue.h>
extern struct android_app *g_App;
#endif

#include "common/lrit/generic_xrit_calibrator.h"

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
        for (int64_t c = 0; c < (int64_t)images.size(); c++)
        {
            savemtx.lock();
            if (images[c].filename.find(".png") == std::string::npos &&
                images[c].filename.find(".jpeg") == std::string::npos &&
                images[c].filename.find(".jpg") == std::string::npos &&
                images[c].filename.find(".j2k") == std::string::npos &&
                images[c].filename.find(".pbm") == std::string::npos)
                images[c].filename += "." + image_format;
            else if (!d_no_not_save_images)
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
            if (images[c].abs_index != -1)
                contents["images"][c]["abs_index"] = images[c].abs_index;

            savemtx.unlock();
            if (!save_as_matrix && !d_no_not_save_images)
                image::save_img(images[c].image, directory + "/" + images[c].filename);
        }

        if (save_as_matrix)
        {
            int size = ceil(sqrt(images.size()));
            logger->debug("Using size %d", size);
            image::Image image_all = image::make_manyimg_composite(size, size, images.size(), [this](int c)
                                                                   { return images[c].image; });
            image::save_img(image_all, directory + "/" + images[0].filename);
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

#ifdef __ANDROID__
        JavaVM *java_vm = g_App->activity->vm;
        JNIEnv *java_env = NULL;

        jint jni_return = java_vm->GetEnv((void **)&java_env, JNI_VERSION_1_6);
        if (jni_return == JNI_ERR)
            throw std::runtime_error("Could not get JNI environement");

        jni_return = java_vm->AttachCurrentThread(&java_env, NULL);
        if (jni_return != JNI_OK)
            throw std::runtime_error("Could not attach to thread");

        jclass activityClass = java_env->FindClass("android/app/NativeActivity");
        jmethodID getCacheDir = java_env->GetMethodID(activityClass, "getCacheDir", "()Ljava/io/File;");
        jobject cache_dir = java_env->CallObjectMethod(g_App->activity->clazz, getCacheDir);

        jclass fileClass = java_env->FindClass("java/io/File");
        jmethodID getPath = java_env->GetMethodID(fileClass, "getPath", "()Ljava/lang/String;");
        jstring path_string = (jstring)java_env->CallObjectMethod(cache_dir, getPath);

        const char *path_chars = java_env->GetStringUTFChars(path_string, NULL);
        std::string tmp_path(path_chars);

        java_env->ReleaseStringUTFChars(path_string, path_chars);
        jni_return = java_vm->DetachCurrentThread();
        if (jni_return != JNI_OK)
            throw std::runtime_error("Could not detach from thread");
#else
        std::string tmp_path = std::filesystem::temp_directory_path().string();
#endif

        image::Image img_matrix;
        if (save_as_matrix)
        {
            if (file.find("http") == 0)
            {
                std::string res;
                if (perform_http_request(directory + "/" + contents["images"][0]["file"].get<std::string>(), res))
                    throw std::runtime_error("Could not download from : " + directory + "/" + contents["images"][0]["file"].get<std::string>());
                std::ofstream(tmp_path + "/satdumpdltmp.tmp", std::ios::binary).write((char *)res.data(), res.size());
                image::load_img(img_matrix, tmp_path + "/satdumpdltmp.tmp");
                if (std::filesystem::exists(tmp_path + "/satdumpdltmp.tmp"))
                    std::filesystem::remove(tmp_path + "/satdumpdltmp.tmp");
            }
            else if (!d_no_not_load_images)
            {
                if (std::filesystem::exists(directory + "/" + contents["images"][0]["file"].get<std::string>()))
                    image::load_img(img_matrix, directory + "/" + contents["images"][0]["file"].get<std::string>());
            }
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
                if (file.find("http") == 0)
                {
                    std::string res;
                    if (perform_http_request(directory + "/" + contents["images"][c]["file"].get<std::string>(), res))
                        throw std::runtime_error("Could not download from : " + directory + "/" + contents["images"][c]["file"].get<std::string>());
                    std::ofstream(tmp_path + "/satdumpdltmp.tmp", std::ios::binary).write((char *)res.data(), res.size());
                    image::load_img(img_holder.image, tmp_path + "/satdumpdltmp.tmp");
                    if (std::filesystem::exists(tmp_path + "/satdumpdltmp.tmp"))
                        std::filesystem::remove(tmp_path + "/satdumpdltmp.tmp");
                }
                else if (!d_no_not_load_images)
                {
                    if (std::filesystem::exists(directory + "/" + contents["images"][c]["file"].get<std::string>()))
                        image::load_img(img_holder.image, directory + "/" + contents["images"][c]["file"].get<std::string>());
                }
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
            if (contents["images"][c].contains("abs_index"))
                img_holder.abs_index = contents["images"][c]["abs_index"].get<int>();

            images.push_back(img_holder);
        }
    }

    void ImageProducts::init_calibration()
    {
        calib_mutex.lock();

        calibration_type_lut.clear();
        calibration_wavenumber_lut.clear();
        for (int i = 0; i < images.size(); i++)
        {
            calibration_type_lut.push_back(get_calibration_type(i));
            calibration_wavenumber_lut.push_back(get_wavenumber(i));
        }

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
            else if (calibrator_id == "generic_xrit")
                calibrator_ptr = std::make_shared<lrit::GenericxRITCalibrator>(contents["calibration"], this);
            else
            {
                logger->error("Requested calibrator " + calibrator_id + " does not exist!");
                calibrator_ptr = std::make_shared<DummyCalibrator>(contents["calibration"], this);
            }

            try
            {
                calibrator_ptr->init();
            }
            catch (std::exception &e)
            {
                logger->error("Could not initialize calibrator: %s", e.what());
                calibrator_ptr = std::make_shared<DummyCalibrator>(contents["calibration"], this);
            }
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
        int calib_index = image_index;
        calib_mutex.lock();
        uint16_t val = images[image_index].image.get(0, x, y) >> (images[image_index].image.depth() - bit_depth);

        double val2 = CALIBRATION_INVALID_VALUE;
        if (images[image_index].abs_index == -2)
        {
            calib_mutex.unlock();
            return val2;
        }
        else if (images[image_index].abs_index != -1)
            calib_index = images[image_index].abs_index;

        if (calibrator_ptr != nullptr)
            val2 = calibrator_ptr->compute(calib_index, x, y, val);
        else if (lua_state_ptr != nullptr)
            val2 = ((sol::function *)lua_comp_func_ptr)->call(calib_index, x, y, val).get<double>();

        if (calibration_type_lut[image_index] == calib_type_t::CALIB_RADIANCE && temp)
            val2 = radiance_to_temperature(val2, calibration_wavenumber_lut[image_index]);

        calib_mutex.unlock();
        return val2;
    }

    image::Image ImageProducts::get_calibrated_image(int image_index, float *progress, calib_vtype_t vtype, std::pair<double, double> range)
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

            image::Image output(images[image_index].image.depth(), images[image_index].image.width(), images[image_index].image.height(), 1);

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

                        if (vtype == CALIB_VTYPE_TEMPERATURE && calibration_type_lut[image_index] == CALIB_RADIANCE)
                            cal_val = radiance_to_temperature(cal_val, wn);

                        output.setf(0, x, y, output.clampf((cal_val - range.first) / abs(range.first - range.second)));
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

    bool image_equation_contains(std::string init, std::string match, int *loc)
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
            {
                if (loc != nullptr)
                    *loc = pos;
                return true;
            }

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

    bool check_composite_from_product_can_be_made(ImageProducts &product, ImageCompositeCfg cfg)
    {
        std::string str_to_find_channels = cfg.equation;
        if (cfg.lut.size() != 0 || cfg.lua.size() != 0 || cfg.cpp.size() != 0)
            str_to_find_channels = cfg.channels;

        std::vector<std::string> channels_present;

        for (size_t i = 0; i < str_to_find_channels.size() - 1; i++)
        {
            if (str_to_find_channels[i + 0] == 'c' && str_to_find_channels[i + 1] == 'c' && str_to_find_channels[i + 2] == 'h')
            {
                std::string final_ch;
                int fpos = i;
                for (size_t ic = i; ic < str_to_find_channels.size(); ic++)
                {
                    char v = str_to_find_channels[ic];
                    if (v < 48 || v > 57)
                        if (v < 65 || v > 90)
                            if (v < 97 || v > 122)
                                break;
                    final_ch += v;
                    fpos = ic;
                }

                bool already_present = false;
                for (auto &str : channels_present)
                    if (str == final_ch)
                        already_present = true;
                if (!already_present && final_ch.size() > 0)
                    channels_present.push_back(final_ch);

                i = fpos;
                continue;
            }

            if (str_to_find_channels[i + 0] == 'c' && str_to_find_channels[i + 1] == 'h')
            {
                std::string final_ch;
                int fpos = i;
                for (size_t ic = i; ic < str_to_find_channels.size(); ic++)
                {
                    char v = str_to_find_channels[ic];
                    if (v < 48 || v > 57)
                        if (v < 65 || v > 90)
                            if (v < 97 || v > 122)
                                break;
                    final_ch += v;
                    fpos = ic;
                }

                bool already_present = false;
                for (auto &str : channels_present)
                    if (str == final_ch)
                        already_present = true;
                if (!already_present && final_ch.size() > 0)
                    channels_present.push_back(final_ch);

                i = fpos;
                continue;
            }
        }

        if (channels_present.size() == 0)
            return false;

        for (int i = 0; i < (int)product.images.size(); i++)
        {
            auto &img = product.images[i];
            std::string equ_str = "ch" + img.channel_name;
            std::string equ_str_calib = "cch" + img.channel_name;

            auto it_ch = std::find(channels_present.begin(), channels_present.end(), equ_str);
            if (it_ch != channels_present.end())
                channels_present.erase(it_ch);

            if (product.has_calibation() && product.get_wavenumber(i) != -1)
            {
                auto it_cch = std::find(channels_present.begin(), channels_present.end(), equ_str_calib);
                if (it_cch != channels_present.end())
                    channels_present.erase(it_cch);
            }
        }

        return channels_present.size() == 0;
    }

    image::Image make_composite_from_product(ImageProducts &product, ImageCompositeCfg cfg, float *progress, std::vector<double> *final_timestamps, nlohmann::json *final_metadata)
    {
        std::vector<int> channel_indexes;
        std::vector<std::string> channel_numbers;
        std::vector<image::Image> images_obj;
        std::map<std::string, int> offsets;

        int max_width_total = 0;
        int max_width_used = 1;
        int min_offset = 100000000;

        if (final_metadata != nullptr)
            final_metadata->clear();

        std::string str_to_find_channels = cfg.equation;

        bool is_lut_or_lua_or_cpp = false;
        if (cfg.lut.size() != 0 || cfg.lua.size() != 0 || cfg.cpp.size() != 0)
        {
            str_to_find_channels = cfg.channels;
            is_lut_or_lua_or_cpp = true;
        }

        bool a_channel_is_empty = false;

        // We want channels in equation order!
        struct TempBeforeSort
        {
            int loc;
            int index;
            std::string number;
            image::Image img;
        };
        std::vector<TempBeforeSort> channel_indexes_locations;

        // Find all channels and prepare them
        for (int i = 0; i < (int)product.images.size(); i++)
        {
            auto &img = product.images[i];
            std::string equ_str = "ch" + img.channel_name;
            std::string equ_str_calib = "cch" + img.channel_name;

            if (max_width_total < (int)img.image.width())
                max_width_total = img.image.width();

            int cal_loc = -1;
            int loc = -1;

            if (image_equation_contains(str_to_find_channels, equ_str_calib, &cal_loc) && product.has_calibation())
            {
                product.init_calibration();
                if (is_lut_or_lua_or_cpp)
                {
                    channel_indexes_locations.push_back(TempBeforeSort());
                    TempBeforeSort &cur = channel_indexes_locations[channel_indexes_locations.size() - 1];
                    cur.loc = cal_loc;
                    cur.index = i;
                    cur.number = equ_str_calib;

                    if (cfg.calib_cfg.contains(equ_str_calib))
                    {
                        ImageProducts::calib_vtype_t type;
                        std::pair<double, double> range;
                        get_calib_cfg_from_json(cfg.calib_cfg[equ_str_calib], type, range);
                        cur.img = product.get_calibrated_image(i, progress, type, range);
                    }
                    else
                    {
                        cur.img = product.get_calibrated_image(i, progress);
                    }
                }
                else
                {
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
                }
                offsets.emplace(equ_str_calib, img.offset_x);
                logger->debug("Composite needs calibrated channel %s", equ_str.c_str());

                if (max_width_used < (int)img.image.width())
                    max_width_used = img.image.width();

                if (min_offset > img.offset_x)
                    min_offset = img.offset_x;

                if (img.image.size() == 0)
                    a_channel_is_empty = true;
            }

            if (image_equation_contains(str_to_find_channels, equ_str, &loc) && cal_loc != loc)
            {
                channel_indexes_locations.push_back(TempBeforeSort());
                if (is_lut_or_lua_or_cpp)
                {
                    TempBeforeSort &cur = channel_indexes_locations[channel_indexes_locations.size() - 1];
                    cur.loc = loc;
                    cur.index = i;
                    cur.number = equ_str;
                    cur.img = img.image;
                }
                else
                {
                    channel_indexes.push_back(i);
                    channel_numbers.push_back(equ_str);
                    images_obj.push_back(img.image);
                }
                offsets.emplace(equ_str, img.offset_x);
                logger->debug("Composite needs channel %s", equ_str.c_str());

                if (max_width_used < (int)img.image.width())
                    max_width_used = img.image.width();

                if (min_offset > img.offset_x)
                    min_offset = img.offset_x;

                if (img.image.size() == 0)
                    a_channel_is_empty = true;
            }
        }

        // They need to be in equation order!
        if (is_lut_or_lua_or_cpp)
        {
            std::sort(channel_indexes_locations.begin(), channel_indexes_locations.end(),
                      [](auto &a, auto &b)
                      {
                          return a.loc < b.loc;
                      });
            for (auto &v : channel_indexes_locations)
            {
                channel_indexes.push_back(v.index);
                channel_numbers.push_back(v.number);
                images_obj.push_back(std::move(v.img));
            }
        }

        // Free up memory
        std::vector<TempBeforeSort>().swap(channel_indexes_locations);

        if (channel_indexes.size() == 0 || a_channel_is_empty)
        {
            logger->error("One or more of the required channels are missing!");
            return image::Image();
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

            if (final_metadata != nullptr && min_offset != 0)
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
                std::vector<image::Image> images_obj_new;
                for (int i = 0; i < (int)channel_indexes.size(); i++)
                    images_obj_new.push_back(image::Image(product.images[channel_indexes[i]].image.depth(),
                                                          product.images[channel_indexes[i]].image.width(),
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
                                // TODOIMG
                                //  Copy over scanlines
                                // memcpy(&images_obj_new[i][y_index * images_obj_new[i].width() * (single_line ? 1 : product.get_ifov_y_size(index))],
                                //        &images_obj[i][t * images_obj_new[i].width() * (single_line ? 1 : product.get_ifov_y_size(index))],
                                //        images_obj_new[i].width() * (single_line ? 1 : product.get_ifov_y_size(index)) * sizeof(uint16_t));
                                // memcpy((uint8_t*)images_obj_new[i].raw_data() + (y_index * images_obj_new[i].width() * (single_line ? 1 : product.get_ifov_y_size(index))) * images_obj_new[i].typesize(),
                                //        (uint8_t*)images_obj[i].raw_data() + (t * images_obj_new[i].width() * (single_line ? 1 : product.get_ifov_y_size(index))) * images_obj[i].typesize(),
                                //        images_obj_new[i].width() * (single_line ? 1 : product.get_ifov_y_size(index)) * images_obj[i].typesize());
                                image::imemcpy(images_obj_new[i], y_index * images_obj_new[i].width() * (single_line ? 1 : product.get_ifov_y_size(index)),
                                               images_obj[i], t * images_obj_new[i].width() * (single_line ? 1 : product.get_ifov_y_size(index)),
                                               images_obj_new[i].width() * (single_line ? 1 : product.get_ifov_y_size(index)));
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

        image::Image rgb_composite;

        try
        {
            if (cfg.cpp != "")
            {
                std::vector<std::function<image::Image(
                    satdump::ImageProducts *,
                    std::vector<image::Image> &,
                    std::vector<std::string>,
                    std::string,
                    nlohmann::json,
                    nlohmann::json,
                    std::vector<double> *,
                    float *)>>
                    compositors;
                satdump::eventBus->fire_event<RequestCppCompositeEvent>({cfg.cpp, compositors, &product});
                if (compositors.size() > 0)
                    rgb_composite = compositors[0](&product, images_obj, channel_numbers, resources::getResourcePath(cfg.lua), cfg.vars, offsets, final_timestamps, progress);
                else
                    logger->error("Could not get a C++ compositor with ID " + cfg.cpp);
            }
            else if (cfg.lua != "")
                rgb_composite = image::generate_composite_from_lua(&product, images_obj, channel_numbers, resources::getResourcePath(cfg.lua), cfg.vars, offsets, final_timestamps, progress);
            else if (cfg.lut != "")
                rgb_composite = image::generate_composite_from_lut(images_obj, channel_numbers, resources::getResourcePath(cfg.lut), offsets, progress);
            else
                rgb_composite = image::generate_composite_from_equ(images_obj, channel_numbers, cfg.equation, offsets, progress);
        }
        catch (std::exception &e)
        {
            logger->error("Error making composite! %s", e.what());
        }

        // Free up memory
        std::vector<image::Image>().swap(images_obj);

        if (cfg.median_blur)
            image::median_blur(rgb_composite);

        if (cfg.despeckle)
            image::kuwahara_filter(rgb_composite);

        if (cfg.equalize)
            image::equalize(rgb_composite);

        if (cfg.individual_equalize)
            image::equalize(rgb_composite, true);

        if (cfg.white_balance)
            image::white_balance(rgb_composite);

        if (cfg.invert)
            image::linear_invert(rgb_composite);

        if (cfg.normalize)
            image::normalize(rgb_composite);

        if (cfg.remove_background)
            image::remove_background(rgb_composite, product.get_proj_cfg(), progress);

        if (cfg.manual_brightness != 0 || cfg.manual_contrast != 0)
            image::brightness_contrast(rgb_composite, cfg.manual_brightness, cfg.manual_contrast);

        if (cfg.apply_lut)
        {
            auto lut_image = image::LUT_jet<uint8_t>();
            rgb_composite.to_rgb();
            rgb_composite = rgb_composite.to_depth(lut_image.depth());
            for (size_t i = 0; i < rgb_composite.width() * rgb_composite.height(); i++)
            {
                int val = rgb_composite.getf(i) * lut_image.width();
                if (val >= lut_image.width())
                    val = lut_image.width() - 1;
                rgb_composite.set(0, i, lut_image.get(0, val));
                rgb_composite.set(1, i, lut_image.get(1, val));
                rgb_composite.set(2, i, lut_image.get(2, val));
            }
        }

        return rgb_composite;
    }

    image::Image perform_geometric_correction(ImageProducts &product, image::Image img, bool &success, float *foward_table)
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
