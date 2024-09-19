#include "image_handler.h"
#include "common/calibration.h"
#include "common/image/image_background.h"

#include "imgui/pfd/pfd_utils.h"
#include "imgui/imgui_internal.h"

#include "common/projection/gcp_compute/gcp_compute.h"
#include "common/projection/warp/warp.h"
#include "common/projection/projs/equirectangular.h"
#include "common/map/map_drawer.h"
#include "resources.h"
#include "core/opencl.h"
#include "common/widgets/switch.h"
#include "common/widgets/stepped_slider.h"
#include "main_ui.h"
#include "common/image/brightness_contrast.h"

#include "common/image/meta.h"
#include "common/projection/reprojector.h"

#include "common/image/processing.h"
#include "common/image/image_lut.h"

namespace satdump
{
    void ImageViewerHandler::init()
    {
        products = (ImageProducts *)ViewerHandler::products;

        if (products->has_calibation())
            products->init_calibration();

        // TMP
        if (instrument_cfg.contains("rgb_composites"))
        {
            for (nlohmann::detail::iteration_proxy_value<nlohmann::detail::iter_impl<nlohmann::ordered_json>> compo : instrument_cfg["rgb_composites"].items())
            {
                if (check_composite_from_product_can_be_made(*products, compo.value().get<ImageCompositeCfg>()))
                    rgb_presets.push_back({compo.key(), compo.value().get<ImageCompositeCfg>()});
                else
                    logger->debug("Disabling " + compo.key() + " as it can't be made!");
            }
        }

        select_image_str += std::string("Composite") + '\0';

        for (int i = 0; i < (int)products->images.size(); i++)
        {
            auto &img = products->images[i];
            select_image_str += "Channel " + img.channel_name + '\0';
            channel_numbers.push_back(img.channel_name);
            radiance_ranges.push_back(products->get_calibration_default_radiance_range(i));
            temp_ranges.push_back({radiance_to_temperature(radiance_ranges[i].first, products->get_wavenumber(i)),
                                   radiance_to_temperature(radiance_ranges[i].second, products->get_wavenumber(i))});
        }

        // generate scale iamge
        updateScaleImage();

        // Setup correction factors.
        updateCorrectionFactors(true);

        lut_image = image::LUT_jet<uint8_t>();
        asyncUpdate();

        try
        {
            overlay_handler.set_config(config::main_cfg["user"]["viewer_state"]["products_handler"][products->instrument_name]["overlay_cfg"], false);
        }
        catch (std::exception &)
        {
        }
    }

    ImageViewerHandler::~ImageViewerHandler()
    {
        handler_thread_pool.stop();
        for (int i = 0; i < handler_thread_pool.size(); i++)
        {
            if (handler_thread_pool.get_thread(i).joinable())
                handler_thread_pool.get_thread(i).join();
        }

        config::main_cfg["user"]["viewer_state"]["products_handler"][products->instrument_name]["overlay_cfg"] = overlay_handler.get_config();
        config::saveUserConfig();
        delete[] scale_buffer;
    }

    void ImageViewerHandler::updateImage()
    {
        if (select_image_id == 0)
        {
            current_image = rgb_image;
        }
        else if (select_image_id - 1 < (int)products->images.size())
        {
            if (active_channel_calibrated && products->has_calibation())
            {
                if (products->get_calibration_type(active_channel_id) && is_temp)
                    current_image = products->get_calibrated_image(select_image_id - 1, &rgb_progress,
                                                                   ImageProducts::CALIB_VTYPE_TEMPERATURE, temp_ranges[select_image_id - 1]);
                else
                    current_image = products->get_calibrated_image(select_image_id - 1, &rgb_progress,
                                                                   ImageProducts::CALIB_VTYPE_AUTO, radiance_ranges[select_image_id - 1]);
                update_needed = false;
            }
            else
                current_image = products->images[select_image_id - 1].image;
            current_proj_metadata = products->get_channel_proj_metdata(select_image_id - 1);
        }

        if (select_image_id != 0)
            current_timestamps = products->get_timestamps(select_image_id - 1);

        if (median_blur)
            image::median_blur(current_image);

        if (despeckle)
            image::kuwahara_filter(current_image);

        if (rotate_image)
            current_image.mirror(true, true);

        if (equalize_image)
            image::equalize(current_image);

        if (individual_equalize_image)
            image::equalize(current_image, true);

        if (white_balance_image)
            image::white_balance(current_image);

        if (invert_image)
            image::linear_invert(current_image);

        if (normalize_image)
            image::normalize(current_image);

        if (manual_brightness_contrast)
            image::brightness_contrast(current_image, manual_brightness_contrast_brightness, manual_brightness_contrast_constrast);

        // TODO : Cleanup?
        if (using_lut)
        {
            if (current_image.channels() < 3)
                current_image.to_rgb();
            if (current_image.depth() != lut_image.depth())
                current_image = current_image.to_depth(lut_image.depth());
            for (size_t i = 0; i < current_image.width() * current_image.height(); i++)
            {
                size_t val = current_image.getf(i) * lut_image.width();
                if (val >= lut_image.width())
                    val = lut_image.width() - 1;
                current_image.set(0, i, lut_image.get(0, val));
                current_image.set(1, i, lut_image.get(1, val));
                current_image.set(2, i, lut_image.get(2, val));
            }
        }

        if (remove_background)
            image::remove_background(current_image, products->get_proj_cfg(), &rgb_progress);

        int pre_corrected_width = current_image.width();
        int pre_corrected_height = current_image.height();

        std::vector<float> corrected_stuff;
        if (correct_image)
        {
            corrected_stuff.resize(current_image.width());
            bool success = false;
            image::Image cor = perform_geometric_correction(*products, current_image, success, corrected_stuff.data());
            if (success)
                current_image = cor;
            if (!success)
                corrected_stuff.clear();
        }

        if (overlay_handler.enabled())
        {
            // Ensure this is RGB!!
            if (current_image.channels() < 3)
                current_image.to_rgb();
            nlohmann::json proj_cfg = products->get_proj_cfg();
            proj_cfg["metadata"] = current_proj_metadata;
            if (products->has_tle())
                proj_cfg["metadata"]["tle"] = products->get_tle();
            if (products->has_timestamps)
                proj_cfg["metadata"]["timestamps"] = current_timestamps;

            bool do_correction = corrected_stuff.size() != 0 && correct_image;
            if (do_correction != last_correct_image || rotate_image != last_rotate_image ||
                current_image.width() != last_width || current_image.height() != last_height || last_proj_cfg != proj_cfg)
            {
                overlay_handler.clear_cache();
                proj_func = satdump::reprojection::setupProjectionFunction(pre_corrected_width,
                                                                           pre_corrected_height,
                                                                           proj_cfg,
                                                                           !do_correction && rotate_image);

                if (do_correction)
                {
                    int fwidth = current_image.width();
                    int fheight = current_image.height();
                    bool rotate = rotate_image;
                    auto &proj_func = this->proj_func;

                    std::function<std::pair<int, int>(double, double, int, int)> newfun =
                        [proj_func, corrected_stuff, fwidth, fheight, rotate](double lat, double lon, int map_height, int map_width) mutable -> std::pair<int, int>
                    {
                        std::pair<int, int> ret = proj_func(lat, lon, map_height, map_width);
                        if (ret.first != -1 && ret.second != -1 && ret.first < (int)corrected_stuff.size() && ret.first >= 0)
                        {
                            ret.first = corrected_stuff[ret.first];
                            if (rotate)
                            {
                                ret.first = (fwidth - 1) - ret.first;
                                ret.second = (fheight - 1) - ret.second;
                            }
                        }
                        else
                            ret.second = ret.first = -1;
                        return ret;
                    };
                    proj_func = newfun;
                }

                last_correct_image = do_correction;
                last_rotate_image = rotate_image;
                last_width = current_image.width();
                last_height = current_image.height();
                last_proj_cfg = proj_cfg;
            }

            overlay_handler.apply(current_image, proj_func);
        }

        image_view.update(current_image);

        // Tooltip function
        image_view.mouseCallback = [this](int x, int y)
        {
            if (active_channel_id >= 0)
            {
                y+=1;
                if (rotate_image)
                {
                    x = current_image.width() - 1 - x;
                    y = current_image.height() - 1 - y;
                }

                int raw_value = products->images[active_channel_id].image.get(0, correct_image ? correction_factors[x] : x, y) >> (products->images[active_channel_id].image.depth() - products->bit_depth);

                ImGui::BeginTooltip();
                ImGui::Text("Count : %d", raw_value);
                if (products->has_calibation() && products->get_wavenumber(active_channel_id) != -1)
                {
                    double radiance = products->get_calibrated_value(active_channel_id, x, y);
                    if (correct_image)
                    {
                        updateCorrectionFactors();
                        radiance = products->get_calibrated_value(active_channel_id, correction_factors[x], y);
                    }
                    if (radiance != CALIBRATION_INVALID_VALUE)
                    {
                        if (products->get_calibration_type(active_channel_id) == products->CALIB_REFLECTANCE)
                        {
                            ImGui::Text("Albedo : %.2f %%", radiance * 100.0);
                        }
                        else
                        {
                            ImGui::Text("Radiance : %.10f", radiance);
                            ImGui::Text("Temperature : %.2f °C", radiance_to_temperature(radiance, products->get_wavenumber(active_channel_id)) - 273.15);
                        }
                    }
                    else
                    {
                        ImGui::Text("Calibration Error! - Invalid Value");
                    }
                }
                ImGui::EndTooltip();

                /*if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                {
                    logger->info("%d, %d", x, y);
                }*/
            }
        };
    }

    void ImageViewerHandler::asyncUpdate()
    {
        handler_thread_pool.clear_queue();
        handler_thread_pool.push([this](int)
                                 {   async_image_mutex.lock();
                                    is_updating = true;
                                    logger->info("Update image...");
                                    updateImage();
                                    logger->info("Done");
                                    is_updating = false;
                                    async_image_mutex.unlock(); });
    }

    void ImageViewerHandler::updateRGB()
    {
        rgb_processing = true;
        active_channel_id = -1;
        handler_thread_pool.clear_queue();
        handler_thread_pool.push([this](int)
                                 { 
                    async_image_mutex.lock();
                    logger->info("Generating RGB Composite");
                    satdump::ImageCompositeCfg cfg; 
                    cfg.equation = rgb_compo_cfg.equation;
                    cfg.lut = rgb_compo_cfg.lut;
                    cfg.channels = rgb_compo_cfg.channels;
                    cfg.lua = rgb_compo_cfg.lua;
                    cfg.cpp = rgb_compo_cfg.cpp;
                    cfg.vars = rgb_compo_cfg.vars;
                    cfg.calib_cfg.clear();

                    if(select_rgb_presets >= 0) 
                    {
                        // median_blur = rgb_compo_cfg.median_blur;
                        // despeckle = rgb_compo_cfg.despeckle;
                        equalize_image = rgb_compo_cfg.equalize;
                        individual_equalize_image = rgb_compo_cfg.individual_equalize;
                        invert_image = rgb_compo_cfg.invert;
                        normalize_image = rgb_compo_cfg.normalize;
                        white_balance_image = rgb_compo_cfg.white_balance;
                        remove_background = rgb_compo_cfg.remove_background;
                        using_lut = rgb_compo_cfg.apply_lut;
                        
                        // Test
                        cfg.calib_cfg = rgb_compo_cfg.calib_cfg;
                    }

                    rgb_image = satdump::make_composite_from_product(*products, cfg, &rgb_progress, &current_timestamps, &current_proj_metadata);
                    select_image_id = 0;
                    updateImage();
                    logger->info("Done");
                    rgb_processing = false; 
                    async_image_mutex.unlock(); });
    }

    void ImageViewerHandler::drawMenu()
    {
        if (ImGui::CollapsingHeader("Image"))
        {
            if (products->images.size() < 100)
            {
                if (ImGui::Combo("##imagechannelcombo", &select_image_id, select_image_str.c_str()))
                {
                    if (select_image_id == 0)
                        active_channel_id = -1;
                    else
                        active_channel_id = select_image_id - 1;
                    asyncUpdate();
                }
            }
            else
            {
                if (ImGui::InputInt("##imagechannelint", &select_image_id))
                {
                    if (select_image_id > (int)products->images.size())
                        select_image_id = (int)products->images.size();
                    else if (select_image_id < 0)
                        select_image_id = 0;

                    if (select_image_id == 0)
                        active_channel_id = -1;
                    else
                        active_channel_id = select_image_id - 1;
                    asyncUpdate();
                }
            }

            if (ImGui::IsItemHovered() && active_channel_id >= 0)
            {
                if (products->get_wavenumber(active_channel_id) != -1)
                {
                    ImGui::BeginTooltip();
                    ImGui::Text("Wavenumber : %f cm\u207b\u00b9", products->get_wavenumber(active_channel_id));
                    double wl_nm = 1e7 / products->get_wavenumber(active_channel_id);
                    double frequency = 299792458.0 / (wl_nm * 10e-10);

                    if (wl_nm < 1e3)
                        ImGui::Text("Wavelength : %f nm", wl_nm);
                    else if (wl_nm < 1e6)
                        ImGui::Text("Wavelength : %f µm", wl_nm / 1e3);
                    else if (wl_nm < 1e9)
                        ImGui::Text("Wavelength : %f mm", wl_nm / 1e6);
                    else
                        ImGui::Text("Wavelength : %f cm", wl_nm / 1e7);

                    if (frequency < 1e3)
                        ImGui::Text("Frequency : %f Hz", frequency);
                    else if (frequency < 1e6)
                        ImGui::Text("Frequency : %f kHz", frequency / 1e3);
                    else if (frequency < 1e9)
                        ImGui::Text("Frequency : %f MHz", frequency / 1e6);
                    else
                        ImGui::Text("Frequency : %f GHz", frequency / 1e9);
                    ImGui::EndTooltip();
                }
            }

            if (products->has_calibation() && active_channel_id >= 0 && products->get_wavenumber(active_channel_id) != -1)
            {
                ImGuiStyle &imgui_style = ImGui::GetStyle();
                int to_pop = 0;
                ImGui::SameLine();
                if (range_window && active_channel_calibrated)
                {
                    ImGui::PushStyleColor(ImGuiCol_Button, imgui_style.Colors[ImGuiCol_TabActive]);
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, imgui_style.Colors[ImGuiCol_TabHovered]);
                    to_pop += 2;
                }
                if (!active_channel_calibrated)
                    ImGui::BeginDisabled();
                if (ImGui::Button(u8"\uf07e"))
                    range_window = !range_window;
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Diaplay Range Control");
                ImGui::PopStyleColor(to_pop);
                to_pop = 0;

                if (show_scale && active_channel_calibrated)
                {
                    ImGui::PushStyleColor(ImGuiCol_Button, imgui_style.Colors[ImGuiCol_TabActive]);
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, imgui_style.Colors[ImGuiCol_TabHovered]);
                    to_pop += 2;
                }
                if (!products->get_calibration_type(active_channel_id))
                    ImGui::BeginDisabled();
                ImGui::SameLine();
                if (ImGui::Button(u8"\uf2c9"))
                    show_scale = !show_scale;
                if (!products->get_calibration_type(active_channel_id))
                    ImGui::EndDisabled();
                ImGui::PopStyleColor(to_pop);
                to_pop = 0;

                if (!active_channel_calibrated)
                    ImGui::EndDisabled();

                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Show Scale");

                if (range_window && active_channel_calibrated)
                {
                    ImGui::Begin("Display Range Control", &range_window, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
                    ImGui::SetWindowSize(ImVec2(200 * ui_scale, 115 * ui_scale));
                    bool buff = false;
                    std::pair<double, double> &this_range = products->get_calibration_type(active_channel_id) && is_temp ? temp_ranges[active_channel_id] : radiance_ranges[active_channel_id];
                    double tmp_min = (this_range.first * (products->get_calibration_type(active_channel_id) ? 1 : 100));
                    double tmp_max = (this_range.second * (products->get_calibration_type(active_channel_id) ? 1 : 100));
                    ImGui::SetNextItemWidth(120 * ui_scale);
                    buff |= ImGui::InputDouble("Minimum", &tmp_min, 0, 0, is_temp && products->get_calibration_type(active_channel_id) ? "%.1f K" : (products->get_calibration_type(active_channel_id) ? "%.2f W·sr\u207b\u00b9·m\u207b\u00b2" : "%.2f%% Albedo"), ImGuiInputTextFlags_EnterReturnsTrue);
                    ImGui::SetNextItemWidth(120 * ui_scale);
                    buff |= ImGui::InputDouble("Maximum", &tmp_max, 0, 0, is_temp && products->get_calibration_type(active_channel_id) ? "%.1f K" : (products->get_calibration_type(active_channel_id) ? "%.2f W·sr\u207b\u00b9·m\u207b\u00b2" : "%.2f%% Albedo"), ImGuiInputTextFlags_EnterReturnsTrue);
                    if (buff)
                    {
                        this_range.first = (tmp_min / (products->get_calibration_type(active_channel_id) ? 1 : 100));
                        this_range.second = (tmp_max / (products->get_calibration_type(active_channel_id) ? 1 : 100));
                        update_needed = true;
                        asyncUpdate();
                    }
                    if (ImGui::Button("Default"))
                    {
                        this_range = is_temp && products->get_calibration_type(active_channel_id) ? std::pair<double, double>{radiance_to_temperature(products->get_calibration_default_radiance_range(active_channel_id).first, products->get_wavenumber(active_channel_id)),
                                                                                                                              radiance_to_temperature(products->get_calibration_default_radiance_range(active_channel_id).second, products->get_wavenumber(active_channel_id))}
                                                                                                  : products->get_calibration_default_radiance_range(active_channel_id);
                        update_needed = true;
                        asyncUpdate();
                    }
                    ImGui::End();
                }

                if (show_scale && active_channel_calibrated && products->get_calibration_type(active_channel_id))
                {
                    if (scale_has_update)
                        updateImageTexture(scale_texture_id, scale_buffer, 25, 512);

                    int w = ImGui::GetWindowSize()[0];
                    if (ImGui::Begin("Scale##scale_window", &show_scale, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize))
                    {
                        ImGui::SetWindowSize(ImVec2(100 * ui_scale, 520 * ui_scale));
                        ImGui::SetWindowPos(ImVec2(w + 20 * ui_scale, 50 * ui_scale), ImGuiCond_Once);

                        ImDrawList *draw_list = ImGui::GetWindowDrawList();
                        float num_height = ImGui::CalcTextSize("0").y;
                        float scale_height = (460 * ui_scale) - num_height;
                        float start_y = ImGui::GetCursorPosY();
                        float graduation_start_y = ImGui::GetCursorScreenPos().y + (num_height / 2);
                        float graduation_start_x = ImGui::GetCursorScreenPos().x + (22 * ui_scale);
                        float graduation_length = imgui_style.ItemSpacing.x * 0.75f;

                        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + num_height / 2);
                        ImGui::Image((void *)(intptr_t)scale_texture_id, ImVec2(22 * ui_scale, scale_height));
                        ImGui::SameLine();
                        ImGui::SetCursorPosY(start_y);
                        ImGui::BeginGroup();

                        std::pair<double, double> actual_ranges = is_temp ? temp_ranges[active_channel_id] : radiance_ranges[active_channel_id];
                        float step_difference = std::abs(actual_ranges.first - actual_ranges.second) / 9;
                        float step_height = (scale_height - 1) / 9;

                        for (int i = 0; i < 10; i++)
                        {
                            ImGui::SetCursorPosY(start_y + i * step_height);
                            ImGui::Text("%.3f", actual_ranges.second - i * step_difference);
                            draw_list->AddLine(ImVec2(graduation_start_x, graduation_start_y + i * step_height),
                                               ImVec2(graduation_start_x + graduation_length, graduation_start_y + i * step_height),
                                               ImGui::ColorConvertFloat4ToU32(imgui_style.Colors[ImGuiCol_TextDisabled]), 1.0 * ui_scale);
                        }

                        ImGui::EndGroup();
                        ImGui::Text(is_temp ? " [K]" : " [W·sr\u207b\u00b9·m\u207b\u00b2]");
                    }
                    ImGui::End();
                }

                ImGui::Spacing();

                ImGui::BeginGroup();
                if (products->get_calibration_type(active_channel_id))
                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3 * ui_scale);
                ImGui::Text("Raw Counts");
                ImGui::SameLine();

                ImGui::SetNextItemWidth(50);
                ToggleButton("##caltog", (int *)&active_channel_calibrated);
                if (ImGui::IsItemClicked())
                    asyncUpdate();
                ImGui::SameLine();
                if (products->get_calibration_type(active_channel_id))
                {
                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 3 * ui_scale);
                    ImGui::SetNextItemWidth(90 * ui_scale);
                    // ImGui::Combo("##temp_rad", &tst, "Radiance\0Temperature");
                    if (ImGui::BeginCombo("##temp_rad", is_temp ? "Temperature" : "Radiance", ImGuiComboFlags_NoArrowButton))
                    {
                        if (ImGui::Selectable("Radiance", !is_temp))
                        {
                            is_temp = false;
                            asyncUpdate();
                        }
                        if (!is_temp)
                            ImGui::SetItemDefaultFocus();

                        if (ImGui::Selectable("Temperature", is_temp))
                        {
                            is_temp = true;
                            asyncUpdate();
                        }
                        if (is_temp)
                            ImGui::SetItemDefaultFocus();
                        ImGui::EndCombo();
                    }
                }
                else
                    ImGui::Text("Albedo");
                ImGui::EndGroup();

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
            }

            if (ImGui::Checkbox("Median Blur", &median_blur))
                asyncUpdate();

            if (ImGui::Checkbox("Despeckle", &despeckle))
                asyncUpdate();

            if (ImGui::Checkbox("Rotate", &rotate_image))
                asyncUpdate();

            if (products->can_geometrically_correct() && ImGui::Checkbox("Correct", &correct_image))
                asyncUpdate();

            if (products->can_remove_background() && ImGui::Checkbox("Remove Background", &remove_background))
                asyncUpdate();

            if (ImGui::Checkbox("Equalize", &equalize_image))
                asyncUpdate();

            if (ImGui::Checkbox("Individual Equalize", &individual_equalize_image))
                asyncUpdate();

            if (ImGui::Checkbox("White Balance", &white_balance_image))
                asyncUpdate();

            if (ImGui::Checkbox("Normalize", &normalize_image))
                asyncUpdate();

            if (ImGui::Checkbox("Invert", &invert_image))
            {
                updateScaleImage();
                asyncUpdate();
            }

            if (ImGui::Checkbox("Apply LUT##lutoption", &using_lut))
            {
                asyncUpdate();
                updateScaleImage();
            }

            if (ImGui::Checkbox("Manual Brightness/Constrast", &manual_brightness_contrast))
                asyncUpdate();
            if (manual_brightness_contrast)
            {
                if (ImGui::SliderFloat("Brightness", &manual_brightness_contrast_brightness, -2, 2))
                    asyncUpdate();
                if (ImGui::SliderFloat("Contrast", &manual_brightness_contrast_constrast, -2, 2))
                    asyncUpdate();
            }

            bool save_disabled = is_updating || rgb_processing;
            if (save_disabled)
                style::beginDisabled();
            if (ImGui::Button("Save"))
            {
                handler_thread_pool.clear_queue();
                handler_thread_pool.push([this](int)
                                         {
                    async_image_mutex.lock();
                    is_updating = true;
                    logger->info("Saving Image...");

                    std::string default_path = config::main_cfg["satdump_directories"]["default_image_output_directory"]["value"].get<std::string>();
                    time_t timevalue = 0;
                    std::string formatted_timestamp = "";
                    std::string filename_suffix = "";
                    int suffix_num = 1;

                    if (products->has_product_timestamp())
                        timevalue = products->get_product_timestamp();
                    else if (products->has_timestamps)
                        timevalue = get_median(products->get_timestamps());

                    if(timevalue != 0)
                    {
                        std::tm *timeReadable = gmtime(&timevalue);
                        formatted_timestamp = std::to_string(timeReadable->tm_year + 1900) + "-" +
                            (timeReadable->tm_mon + 1 > 9 ? std::to_string(timeReadable->tm_mon + 1) : "0" + std::to_string(timeReadable->tm_mon + 1)) + "-" +
                            (timeReadable->tm_mday > 9 ? std::to_string(timeReadable->tm_mday) : "0" + std::to_string(timeReadable->tm_mday)) + "_" +
                            (timeReadable->tm_hour > 9 ? std::to_string(timeReadable->tm_hour) : "0" + std::to_string(timeReadable->tm_hour)) + "-" +
                            (timeReadable->tm_min > 9 ? std::to_string(timeReadable->tm_min) : "0" + std::to_string(timeReadable->tm_min)) + "-" +
                            (timeReadable->tm_sec > 9 ? std::to_string(timeReadable->tm_sec) : "0" + std::to_string(timeReadable->tm_sec)) + "Z_";
                    }

                    std::string file_name = formatted_timestamp + products->instrument_name + "_" +
                        (select_image_id != 0 ? ("ch" + channel_numbers[select_image_id - 1]) :
                        (std::string("composite_") + (select_rgb_presets == -1 ? "custom" : rgb_presets[select_rgb_presets].first)));

                    // To ensure composites with slashes or spaces don't end up in the filename
                    std::replace(file_name.begin(), file_name.end(), ' ', '-');
                    std::replace(file_name.begin(), file_name.end(), '/', '-');

                    // Make sure we never overwrite anything unintentinally
                    while (std::filesystem::exists(default_path + "/" + file_name + filename_suffix + "." + viewer_app->save_type))
                        filename_suffix = "_" + std::to_string(++suffix_num);
                    file_name += filename_suffix;

                    std::string saved_at = save_image_dialog(file_name, default_path, "Save Image", &current_image, &viewer_app->save_type);

                    if (saved_at == "")
                        logger->info("Save cancelled");
                    else
                        logger->info("Saved current image at %s", saved_at.c_str());
                    is_updating = false;
                    async_image_mutex.unlock(); });
            }
            if (save_disabled)
            {
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                    ImGui::SetTooltip("Updating, please wait...");
                style::endDisabled();
            }
        }

        if (ImGui::CollapsingHeader("RGB Composites"))
        {
            bool show_info_button = select_rgb_presets != -1 && rgb_compo_cfg.description_markdown != "";
            if (ImGui::BeginCombo(show_info_button ? "##presetcombo" : "Preset##presetcombo",
                                  select_rgb_presets == -1 ? "" : rgb_presets[select_rgb_presets].first.c_str()))
            {
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::InputTextWithHint("##searchpresets", u8"\uf422   Search", &preset_search_str);
                for (size_t i = 0; i < rgb_presets.size(); i++)
                {
                    bool show = true;
                    if (preset_search_str.size() != 0)
                        show = isStringPresent(rgb_presets[i].first, preset_search_str);

                    if (show && ImGui::Selectable(rgb_presets[i].first.c_str(), (int)i == select_rgb_presets))
                    {
                        select_rgb_presets = i;
                        rgb_compo_cfg = rgb_presets[select_rgb_presets].second;
                        updateRGB();
                    }
                }
                ImGui::EndCombo();
            }

            if (show_info_button)
            {
                ImGui::SameLine();

                if (ImGui::Button(u8"\uf449 Info###compopresetinfo"))
                {
                    std::ifstream ifs(resources::getResourcePath(rgb_compo_cfg.description_markdown));
                    std::string desc_markdown((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
                    markdown_composite_info.set_md(desc_markdown);
                    show_markdown_description = true;
                }
            }

            if (ImGui::InputText("##rgbEquation", &rgb_compo_cfg.equation))
            {
                select_rgb_presets = -1; // Editing, NOT the compo anymore!
                rgb_compo_cfg.cpp = "";
                rgb_compo_cfg.lua = "";
                rgb_compo_cfg.lut = "";
            }
            if (rgb_processing)
                style::beginDisabled();
            if (ImGui::Button("Apply") && !rgb_processing)
            {
                updateRGB();
                return; // Avoid causing ImGui issues!
            }
            if (rgb_processing)
                style::endDisabled();
            ImGui::SameLine();
            ImGui::ProgressBar(rgb_progress);
        }

#if 0
        if (ImGui::CollapsingHeader("Products"))
        {
            if (products->has_calibation())
            {
                if (ImGui::Button("Temperature"))
                {
                    active_channel_id = select_image_id - 1;

                    rgb_image = products->images[active_channel_id].image;
                    rgb_image.to_rgb();

                    for (size_t y = 0; y < products->images[active_channel_id].image.height(); y++)
                    {
                        for (size_t x = 0; x < products->images[active_channel_id].image.width(); x++)
                        {
                            float temp_c = radiance_to_temperature(products->get_calibrated_value(active_channel_id, x, y), products->get_wavenumber(active_channel_id)) - 273.15;

                            image::Image<uint16_t> lut = image::LUT_jet<uint16_t>();

                            int value = ((temp_c + 10) / 15) * 256;

                            if (value < 0)
                                value = 0;
                            if (value > 255)
                                value = 255;

                            rgb_image.channel(0)[y * rgb_image.width() + x] = lut.channel(0)[value];
                            rgb_image.channel(1)[y * rgb_image.width() + x] = lut.channel(1)[value];
                            rgb_image.channel(2)[y * rgb_image.width() + x] = lut.channel(2)[value];
                        }
                    }

                    select_image_id = 0;
                    asyncUpdate();
                }
            }
        }
#endif

        if (products->has_proj_cfg())
        {
            if (ImGui::CollapsingHeader("Map Overlay"))
            {
                if (overlay_handler.drawUI())
                    asyncUpdate();
            }

            if (ImGui::CollapsingHeader("Projection"))
            {
                ImGui::BeginGroup();
                if (!canBeProjected())
                    style::beginDisabled();
                if (ImGui::Button("Add to Projections"))
                    addCurrentToProjections();
                proj_notif.draw();
                ImGui::Checkbox("Old Algorithm", &projection_use_old_algo);
                if (!canBeProjected())
                    style::endDisabled();

                ImGui::EndGroup();
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && !canBeProjected())
                {
                    ImGui::BeginTooltip();
                    if (current_timestamps.size() == 0)
                        ImGui::TextColored(style::theme.red, "No timestamps!");
                    else if (correct_image)
                        ImGui::TextColored(style::theme.red, "Disable correction!");
                    else
                        ImGui::TextColored(style::theme.yellow, "The old algorithm will\n"
                                                                "deal with very bad (noisy) data\n"
                                                                "better.\n"
                                                                "The new one is preferred if\n"
                                                                "possible though, as results\n"
                                                                "are a lot nicer! :-)\n"
                                                                "If you had to use this\n"
                                                                "and the data was not that bad\n"
                                                                "please report as a bug!");

                    ImGui::EndTooltip();
                }
            }
        }

        if (show_markdown_description)
        {
            ImGuiIO &io = ImGui::GetIO();
            ImGui::SetNextWindowSize({400 * ui_scale, 400 * ui_scale}, ImGuiCond_Appearing);
            ImGui::SetNextWindowPos(ImVec2((io.DisplaySize.x / 2) - (400 * ui_scale / 2), (io.DisplaySize.y / 2) - (400 * ui_scale / 2)), ImGuiCond_Appearing);
            ImGui::Begin("Composite Info", &show_markdown_description, ImGuiWindowFlags_NoSavedSettings);
            markdown_composite_info.render();
            ImGui::End();
        }
    }

    void ImageViewerHandler::drawContents(ImVec2 win_size)
    {
        image_view.draw(win_size);
    }

    float ImageViewerHandler::drawTreeMenu()
    {
        tree_local.start();

#if 0
        for (std::pair<std::string, ImageCompositeCfg> &compo : rgb_presets)
        {
            ImGui::TreeNodeEx(std::string("RGB " + compo.first).c_str(), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
            if (ImGui::IsItemClicked())
            {
                rgb_compo_cfg = compo.second;
                updateRGB();
            }
            tree_local.node();
        }
#endif

        return tree_local.end();
    }

    void ImageViewerHandler::updateScaleImage()
    {
        scale_image = image::Image(16, 25, 512, 3);
        for (size_t i = 0; i < 512; i++)
        {
            std::vector<double> color = {double((511 - i) << 7) / 65535.0, double((511 - i) << 7) / 65535.0, double((511 - i) << 7) / 65535.0};
            if (using_lut)
            {
                uint16_t val = color[0] * lut_image.width();
                if (val >= lut_image.width())
                    val = lut_image.width() - 1;
                color[0] = lut_image.getf(0, val);
                color[1] = lut_image.getf(1, val);
                color[2] = lut_image.getf(2, val);
            }

            for (int x = 0; x < 25; x++)
                scale_image.draw_pixel(x, i, color);
        }

        if (invert_image)
            scale_image.mirror(false, true);

        if (scale_texture_id == 0)
        {
            scale_texture_id = makeImageTexture();
            scale_buffer = new uint32_t[25 * 512];
        }

        image::image_to_rgba(scale_image, scale_buffer);
        scale_has_update = true;
    }

    void ImageViewerHandler::updateCorrectionFactors(bool first)
    {
        if (first)
        {
            if (products->images.size() > 0)
            {
                correction_factors = generate_horizontal_corr_lut(*products, products->images[0].image.width());
                correction_factors.push_back(products->images[0].image.width());
            }
        }
        else
        {
            if (correction_factors[correction_factors.size() - 1] != (int)products->images[active_channel_id].image.width())
            {
                correction_factors = generate_horizontal_corr_lut(*products, products->images[active_channel_id].image.width());
                correction_factors.push_back(products->images[active_channel_id].image.width());
            }
        }
    }

    bool ImageViewerHandler::canBeProjected()
    {
        return products->has_proj_cfg() &&
               // products->has_tle() &&
               // products->has_proj_cfg() &&
               // current_timestamps.size() > 0 &&
               // Perhaps we need to check based on the projection type or such?
               !correct_image;
    }

    void ImageViewerHandler::addCurrentToProjections()
    {
        if (canBeProjected())
        {
            try
            {
                // Get projection information
                nlohmann::json proj_cfg;
                proj_cfg = products->get_proj_cfg();
                proj_cfg["metadata"] = current_proj_metadata;
                if (products->has_tle())
                    proj_cfg["metadata"]["tle"] = products->get_tle();
                if (products->has_timestamps)
                    proj_cfg["metadata"]["timestamps"] = current_timestamps;
                image::set_metadata_proj_cfg(current_image, proj_cfg);

                // Create projection title
                std::string timestring, object_name, instrument_name, composite_name;
                if (current_timestamps.size() > 0)
                    timestring = "[" + timestamp_to_string(get_median(current_timestamps)) + "] ";
                else
                    timestring = "";
                if (instrument_cfg.contains("name"))
                    instrument_name = instrument_cfg["name"];
                else
                    instrument_name = products->instrument_name;
                if (products->has_tle())
                    object_name = products->get_tle().name;
                else
                    object_name = "";
                if (timestring != "" || object_name != "")
                    object_name += "\n";
                if (active_channel_id >= 0)
                    composite_name = "Channel " + channel_numbers[active_channel_id];
                else if (select_rgb_presets == -1)
                    composite_name = "Custom (" + rgb_compo_cfg.equation + ")";
                else
                    composite_name = rgb_presets[select_rgb_presets].first;

                // Add projection layer and settings
                viewer_app->projection_layers.push_front({timestring + object_name + instrument_name + " - " + composite_name, current_image});

                if (rotate_image)
                    viewer_app->projection_layers.front().img.mirror(true, true);
                if (projection_use_old_algo)
                    viewer_app->projection_layers.front().old_algo = true;

                proj_notif.set_message(style::theme.green, "Added!");
            }
            catch (std::exception &e)
            {
                logger->error("Could not project image! %s", e.what());
            }
        }
        else
        {
            logger->error("Current image can't be projected!");
        }
    }
}
