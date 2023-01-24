#include "image_handler.h"

#include "core/config.h"
#include "common/calibration.h"

#include "imgui/imgui_internal.h"

#include "common/projection/gcp_compute/gcp_compute.h"
#include "common/projection/warp/warp.h"
#include "common/projection/projs/equirectangular.h"
#include "common/map/map_drawer.h"
#include "resources.h"
#include "common/projection/reprojector.h"
#include "core/opencl.h"
#include "common/widgets/switch.h"

namespace satdump
{
    void ImageViewerHandler::init()
    {
        products = (ImageProducts *)ViewerHandler::products;

        if (products->has_calibation())
            products->init_calibration();

        // TMP
        if (instrument_cfg.contains("rgb_composites"))
            for (nlohmann::detail::iteration_proxy_value<nlohmann::detail::iter_impl<nlohmann::ordered_json>> compo : instrument_cfg["rgb_composites"].items())
                rgb_presets.push_back({compo.key(), compo.value().get<ImageCompositeCfg>()});

        select_image_str += std::string("Composite") + '\0';

        for (int i = 0; i < (int)products->images.size(); i++)
        {
            auto img = products->images[i];
            select_image_str += "Channel " + img.channel_name + '\0';
            channel_numbers.push_back(img.channel_name);
            images_obj.push_back(img.image);
            disaplay_ranges.push_back(products->get_calibration_default_radiance_range(i));
        }

        for (std::pair<std::string, ImageCompositeCfg> &compo : rgb_presets)
            rgb_presets_str += compo.first + '\0';

        // generate scale iamge
        scale_image = image::Image<uint16_t>(25, 512, 3);
        for (int i = 0; i < 512; i++)
        {
            for (int x = 0; x < 25; x++)
            {
                uint16_t color[3] = {static_cast<uint16_t>((511 - i) << 7), static_cast<uint16_t>((511 - i) << 7), static_cast<uint16_t>((511 - i) << 7)};
                scale_image.draw_pixel(x, i, color);
            }
        }
        scale_view.update(scale_image);
        scale_view.allow_zoom_and_move = false;

        asyncUpdate();

#ifdef USE_OPENCL
        opencl::initOpenCL();
        use_draw_proj_algo = opencl::getAllDevices().size() == 0;
#else
        use_draw_proj_algo = true;
#endif
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
                current_image = products->get_calibrated_image(select_image_id - 1,
                                                               &rgb_progress,
                                                               is_temp ? ImageProducts::CALIB_VTYPE_TEMPERATURE : ImageProducts::CALIB_VTYPE_AUTO,
                                                               disaplay_ranges[select_image_id - 1]);
                update_needed = false;
            }
            else
                current_image = products->images[select_image_id - 1].image;
            current_proj_metadata = products->get_channel_proj_metdata(select_image_id - 1);
        }

        if (select_image_id != 0)
            current_timestamps = products->get_timestamps(select_image_id - 1);

        if (median_blur)
            current_image.median_blur();

        if (rotate_image)
            current_image.mirror(true, true);

        if (equalize_image)
            current_image.equalize();

        if (white_balance_image)
            current_image.white_balance();

        if (invert_image)
            current_image.linear_invert();

        if (normalize_image)
            current_image.normalize();

        int pre_corrected_width = current_image.width();
        int pre_corrected_height = current_image.height();

        std::vector<float> corrected_stuff;
        if (correct_image)
        {
            corrected_stuff.resize(current_image.width());
            bool success = false;
            image::Image<uint16_t> cor = perform_geometric_correction(*products, current_image, success, corrected_stuff.data());
            if (success)
                current_image = cor;
            if (!success)
                corrected_stuff.clear();
        }

        if (map_overlay || cities_overlay)
        {
            current_image.to_rgb(); // Ensure this is RGB!!
            auto proj_func = satdump::reprojection::setupProjectionFunction(pre_corrected_width,
                                                                            pre_corrected_height,
                                                                            products->get_proj_cfg(),
                                                                            current_proj_metadata,
                                                                            products->get_tle(),
                                                                            current_timestamps,
                                                                            !(corrected_stuff.size() != 0 && correct_image) && rotate_image);

            if (corrected_stuff.size() != 0 && correct_image)
            {
                int fwidth = current_image.width();
                int fheight = current_image.height();
                bool rotate = rotate_image;

                std::function<std::pair<int, int>(float, float, int, int)> newfun =
                    [proj_func, corrected_stuff, fwidth, fheight, rotate](float lat, float lon, int map_height, int map_width) mutable -> std::pair<int, int>
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

            if (map_overlay)
            {
                logger->info("Drawing map overlay...");
                unsigned short color[3] = {(unsigned short)(color_borders.x * 65535.0f), (unsigned short)(color_borders.y * 65535.0f), (unsigned short)(color_borders.z * 65535.0f)};
                map::drawProjectedMapShapefile({resources::getResourcePath("maps/ne_10m_admin_0_countries.shp")},
                                               current_image,
                                               color,
                                               proj_func,
                                               100);
            }
            if (cities_overlay)
            {
                logger->info("Drawing map overlay...");
                unsigned short color[3] = {(unsigned short)(color_cities.x * 65535.0f), (unsigned short)(color_cities.y * 65535.0f), (unsigned short)(color_cities.z * 65535.0f)};
                map::drawProjectedCapitalsGeoJson({resources::getResourcePath("maps/ne_10m_populated_places_simple.json")},
                                                  current_image,
                                                  color,
                                                  proj_func,
                                                  cities_scale);
            }
        }

        projection_ready = false;

        image_view.update(current_image);
        // current_image.clear();

        // Tooltip function
        image_view.mouseCallback = [this](int x, int y)
        {
            if (active_channel_id >= 0)
            {
                if (rotate_image)
                {
                    x = current_image.width() - 1 - x;
                    y = current_image.height() - 1 - y;
                }

                int raw_value = products->images[active_channel_id].image[y * current_image.width() + x] >> (16 - products->bit_depth);

                ImGui::BeginTooltip();
                ImGui::Text("Count : %d", raw_value);
                if (products->has_calibation())
                {
                    double radiance = products->get_calibrated_value(active_channel_id, x, y);
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
                ImGui::EndTooltip();

                /*if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                {
                    logger->info("{:d}, {:d}", x, y);
                }*/
            }
        };
    }

    void ImageViewerHandler::asyncUpdate()
    {
        ui_thread_pool.push([this](int)
                            {   async_image_mutex.lock();
                                    logger->info("Update image...");
                                    updateImage();
                                    logger->info("Done");
                                    async_image_mutex.unlock(); });
    }

    void ImageViewerHandler::updateRGB()
    {
        rgb_processing = true;
        active_channel_id = -1;
        ui_thread_pool.push([this](int)
                            { 
                    async_image_mutex.lock();
                    logger->info("Generating RGB Composite");
                    satdump::ImageCompositeCfg cfg;
                    cfg.equation = rgb_compo_cfg.equation;
                    cfg.lut = rgb_compo_cfg.lut;
                    cfg.channels = rgb_compo_cfg.channels;
                    cfg.lua = rgb_compo_cfg.lua;
                    cfg.lua_vars = rgb_compo_cfg.lua_vars;

                    equalize_image = rgb_compo_cfg.equalize;
                    invert_image = rgb_compo_cfg.invert;
                    normalize_image = rgb_compo_cfg.normalize;
                    white_balance_image = rgb_compo_cfg.white_balance;

                    rgb_image = satdump::make_composite_from_product(*products, cfg, &rgb_progress, &current_timestamps, &current_proj_metadata);//image::generate_composite_from_equ(images_obj, channel_numbers, rgb_equation, nlohmann::json(), &rgb_progress);
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
                if (products->get_wavenumber(active_channel_id) != 0)
                {
                    ImGui::BeginTooltip();
                    ImGui::Text("Wavenumber : %f cm^-1", products->get_wavenumber(active_channel_id));
                    double wl_nm = 1e7 / products->get_wavenumber(active_channel_id);
                    double frequency = 299792458.0 / (wl_nm * 10e-10);

                    if (wl_nm < 1e3)
                        ImGui::Text("Wavelength : %f nm", wl_nm);
                    else if (wl_nm < 1e6)
                        ImGui::Text("Wavelength : %f um", wl_nm / 1e3);
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

            if (products->has_calibation() && active_channel_id >= 0)
            {
                ImGui::SameLine();
                if (range_window && active_channel_calibrated)
                {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.165, 0.31, 0.51, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.22, 0.482, 0.796, 1.0f));
                }
                else
                {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.216, 0.216, 0.216, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.235, 0.235, 0.235, 1.0f));
                }
                if (!active_channel_calibrated)
                    ImGui::BeginDisabled();
                if (ImGui::Button(u8"\uf07e"))
                    range_window = !range_window;
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Diaplay Range Control");

                if (show_scale && active_channel_calibrated)
                {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.165, 0.31, 0.51, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.22, 0.482, 0.796, 1.0f));
                }
                else
                {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.216, 0.216, 0.216, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.235, 0.235, 0.235, 1.0f));
                }
                if (!products->get_calibration_type(active_channel_id))
                    ImGui::BeginDisabled();
                ImGui::SameLine();
                if (ImGui::Button(u8"\uf2c9"))
                    show_scale = !show_scale;
                if (!products->get_calibration_type(active_channel_id))
                    ImGui::EndDisabled();

                if (!active_channel_calibrated)
                    ImGui::EndDisabled();

                ImGui::PopStyleColor(4);

                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Show Scale");

                if (range_window && active_channel_calibrated)
                {
                    ImGui::Begin("Display Range Control", &range_window, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
                    ImGui::SetWindowSize(ImVec2(200 * ui_scale, 115 * ui_scale));
                    bool buff = false;
                    double tmp_min = is_temp && products->get_calibration_type(active_channel_id) ? radiance_to_temperature(disaplay_ranges[active_channel_id].first, products->get_wavenumber(active_channel_id)) : (disaplay_ranges[active_channel_id].first * (products->get_calibration_type(active_channel_id) ? 1 : 100));
                    double tmp_max = is_temp && products->get_calibration_type(active_channel_id) ? radiance_to_temperature(disaplay_ranges[active_channel_id].second, products->get_wavenumber(active_channel_id)) : (disaplay_ranges[active_channel_id].second * (products->get_calibration_type(active_channel_id) ? 1 : 100));
                    ImGui::SetNextItemWidth(120 * ui_scale);
                    buff |= ImGui::InputDouble("Minium", &tmp_min, 0, 0, is_temp && products->get_calibration_type(active_channel_id) ? "%.1f K" : (products->get_calibration_type(active_channel_id) ? "%.2f W·sr-1·m-2" : "%.2f%% Albedo"), ImGuiInputTextFlags_EnterReturnsTrue);
                    ImGui::SetNextItemWidth(120 * ui_scale);
                    buff |= ImGui::InputDouble("Maximum", &tmp_max, 0, 0, is_temp && products->get_calibration_type(active_channel_id) ? "%.1f K" : (products->get_calibration_type(active_channel_id) ? "%.2f W·sr-1·m-2" : "%.2f%% Albedo"), ImGuiInputTextFlags_EnterReturnsTrue);
                    if (buff)
                    {
                        disaplay_ranges[active_channel_id].first = is_temp && products->get_calibration_type(active_channel_id) ? temperature_to_radiance(tmp_min, products->get_wavenumber(active_channel_id)) : (tmp_min / (products->get_calibration_type(active_channel_id) ? 1 : 100));
                        disaplay_ranges[active_channel_id].second = is_temp && products->get_calibration_type(active_channel_id) ? temperature_to_radiance(tmp_max, products->get_wavenumber(active_channel_id)) : (tmp_max / (products->get_calibration_type(active_channel_id) ? 1 : 100));
                        update_needed = true;
                        asyncUpdate();
                    }
                    if (ImGui::Button("Default"))
                    {
                        disaplay_ranges[active_channel_id] = products->get_calibration_default_radiance_range(active_channel_id);
                        update_needed = true;
                        asyncUpdate();
                    }
                    ImGui::End();
                }

                if (show_scale && active_channel_calibrated && products->get_calibration_type(active_channel_id))
                {
                    int w = ImGui::GetWindowSize()[0];
                    ImGui::Begin("Scale##scale_window", &show_scale, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
                    ImGui::SetWindowSize(ImVec2(100 * ui_scale, 520 * ui_scale));
                    ImGui::SetWindowPos(ImVec2(w + 20 * ui_scale, 50 * ui_scale), ImGuiCond_Once);
                    scale_view.draw(ImVec2(22 * ui_scale, 460 * ui_scale));
                    ImGui::SameLine();
                    ImGui::BeginGroup();
                    int y = ImGui::GetCursorPosY();
                    for (int i = 0; i < 10; i++)
                    {
                        ImGui::SetCursorPosY(y + i * 49 * ui_scale);
                        std::pair<double, double> actual_ranges = disaplay_ranges[active_channel_id];
                        if (is_temp)
                            actual_ranges = {radiance_to_temperature(actual_ranges.first, products->get_wavenumber(active_channel_id)), radiance_to_temperature(actual_ranges.second, products->get_wavenumber(active_channel_id))};
                        ImGui::Text("%.3f", actual_ranges.second - (double)i * abs(actual_ranges.first - actual_ranges.second) / 9.0);
                    }
                    ImGui::EndGroup();
                    ImGui::Text(is_temp ? " [K]" : " [W·sr-1·m-2]");
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
                // ImGui::Text(products->get_calibration_type(active_channel_id) ? "Radiance" : "Albedo");
                if (products->get_calibration_type(active_channel_id))
                {
                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 3 * ui_scale);
                    ImGui::SetNextItemWidth(90 * ui_scale);
                    // ImGui::Combo("##temp_rad", &tst, "Radiance\0Temperature");
                    if (ImGui::BeginCombo("##temp_rad", is_temp ? "Temperature" : "Radiance", ImGuiComboFlags_NoArrowButton))
                    {
                        if (ImGui::Selectable("Radiance", !is_temp))
                        {
                            asyncUpdate();
                            is_temp = false;
                        }
                        if (!is_temp)
                            ImGui::SetItemDefaultFocus();

                        if (ImGui::Selectable("Temperature", is_temp))
                        {
                            asyncUpdate();
                            is_temp = true;
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

            if (ImGui::Checkbox("Rotate", &rotate_image))
                asyncUpdate();

            if (products->can_geometrically_correct())
            {
                if (shouldProject())
                    style::beginDisabled();
                if (ImGui::Checkbox("Correct", &correct_image))
                    asyncUpdate();
                if (shouldProject())
                {
                    style::endDisabled();
                    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                    {
                        ImGui::BeginTooltip();
                        ImGui::TextColored(ImColor(255, 0, 0), "Disable projection!");
                        ImGui::EndTooltip();
                    }
                }
            }

            if (ImGui::Checkbox("Equalize", &equalize_image))
                asyncUpdate();

            if (ImGui::Checkbox("White Balance", &white_balance_image))
                asyncUpdate();

            if (ImGui::Checkbox("Normalize", &normalize_image))
                asyncUpdate();

            if (ImGui::Checkbox("Invert", &invert_image))
                asyncUpdate();

            if (ImGui::Button("Save"))
            {
                std::string default_name = products->instrument_name + "_" + (select_image_id == 0 ? "composite" : ("ch" + channel_numbers[select_image_id - 1])) + ".png";

#ifndef __ANDROID__
                auto result = pfd::save_file("Save Image", default_name, {"*.png"});
                while (!result.ready(1000))
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));

                if (result.result().size() > 0)
                {
                    std::string path = result.result();
                    logger->info("Saving current image at {:s}", path.c_str());
                    current_image.save_img(path);
                }
#else
                std::string path = "/storage/emulated/0/" + default_name;
                logger->info("Saving current image at {:s}", path.c_str());
                current_image.save_img("" + path);
#endif
            }
        }

        if (ImGui::CollapsingHeader("RGB Composites"))
        {
            if (ImGui::Combo("Preset", &select_rgb_presets, rgb_presets_str.c_str()))
            {
                rgb_compo_cfg = rgb_presets[select_rgb_presets].second;
                updateRGB();
            }

            ImGui::InputText("##rgbEquation", &rgb_compo_cfg.equation);
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
                if (ImGui::Checkbox("Borders", &map_overlay))
                    asyncUpdate();
                ImGui::SameLine();
                ImGui::ColorEdit3("##borders", (float *)&color_borders, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
                if (ImGui::Checkbox("Cities", &cities_overlay))
                    asyncUpdate();
                ImGui::SameLine();
                ImGui::ColorEdit3("##cities", (float *)&color_cities, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
                ImGui::SliderFloat("Cities Scale", &cities_scale, 0.1, 10);
            }

            if (ImGui::CollapsingHeader("Projection"))
            {
                ImGui::BeginGroup();
                if (!canBeProjected())
                    style::beginDisabled();
                ImGui::Checkbox("Project", &should_project);
                if (!canBeProjected())
                    style::endDisabled();

                ImGui::SameLine();

                if (!should_project)
                    style::beginDisabled();
                ImGui::Checkbox("Old algorithm", &use_draw_proj_algo);
                if (!should_project)
                    style::endDisabled();
                ImGui::EndGroup();
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                {
                    ImGui::BeginTooltip();
                    if (current_timestamps.size() == 0)
                        ImGui::TextColored(ImColor(255, 0, 0), "No timestamps!");
                    else if (correct_image)
                        ImGui::TextColored(ImColor(255, 0, 0), "Disable correction!");
                    else
                        ImGui::TextColored(ImColor(255, 255, 0), "The old algorithm will\n"
                                                                 "deal with bad (noisy) data\n"
                                                                 "better, and is also faster \n"
                                                                 "if you do not have an\n"
                                                                 "OpenCL-compatible Graphics\n"
                                                                 "Card.\n"
                                                                 "The new one is preferred if\n"
                                                                 "possible though, as results\n"
                                                                 "are a lot nicer! :-)");
                    ImGui::EndTooltip();
                }
            }
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

    bool ImageViewerHandler::canBeProjected()
    {
        return products->has_proj_cfg() &&
               products->has_tle() &&
               products->has_proj_cfg() &&
               current_timestamps.size() > 0 &&
               !correct_image;
    }

    bool ImageViewerHandler::hasProjection()
    {
        return projection_ready && should_project;
    }

    bool ImageViewerHandler::shouldProject()
    {
        return should_project;
    }

    void ImageViewerHandler::updateProjection(int width, int height, nlohmann::json settings, float *progess)
    {
        if (canBeProjected())
        {
            reprojection::ReprojectionOperation op;
            op.source_prj_info = products->get_proj_cfg();
            op.source_mtd_info = current_proj_metadata;
            op.target_prj_info = settings;
            op.img = current_image;
            if (rotate_image)
                op.img.mirror(true, true);
            op.output_width = width;
            op.output_height = height;
            op.use_draw_algorithm = use_draw_proj_algo;
            op.img_tle = products->get_tle();
            op.img_tim = current_timestamps;
            reprojection::ProjectionResult res = reprojection::reproject(op, progess);
            projected_img = res.img;
            projection_ready = true;
        }
        else
        {
            logger->error("Current image can't be projected!");
        }
    }

    image::Image<uint16_t> &ImageViewerHandler::getProjection()
    {
        return projected_img;
    }
}