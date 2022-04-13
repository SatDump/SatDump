#include "image_handler.h"

#include "core/config.h"
#include "common/calibration.h"

namespace satdump
{
    void ImageViewerHandler::init()
    {
        products = (ImageProducts *)ViewerHandler::products;

        // TMP
        if (instrument_cfg.contains("rgb_composites"))
            for (nlohmann::detail::iteration_proxy_value<nlohmann::detail::iter_impl<nlohmann::ordered_json>> compo : instrument_cfg["rgb_composites"].items())
                rgb_presets.push_back({compo.key(), compo.value()["equation"].get<std::string>()});

        select_image_str += std::string("Composite") + '\0';

        for (int i = 0; i < products->images.size(); i++)
        {
            auto img = products->images[i];
            select_image_str += "Channel " + std::get<1>(img) + '\0';
            channel_numbers.push_back(std::get<1>(img));
            images_obj.push_back(std::get<2>(img));
        }

        for (std::pair<std::string, std::string> &compo : rgb_presets)
            rgb_presets_str += compo.first + '\0';

        asyncUpdate();
    }

    void ImageViewerHandler::updateImage()
    {
        if (select_image_id == 0)
            current_image = rgb_image;
        else
            current_image = std::get<2>(products->images[select_image_id - 1]);

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

        image_view.update(current_image);
        // current_image.clear();

        // Tooltip function
        image_view.mouseCallback = [this](int x, int y)
        {
            if (select_image_id > 0)
            {

                int raw_value = std::get<2>(products->images[select_image_id - 1])[y * current_image.width() + x] >> (16 - products->bit_depth);
                double radiance = products->get_radiance_value(select_image_id - 1, x, y);

                ImGui::BeginTooltip();
                ImGui::Text("Count : %d", raw_value);
                if (products->has_calibation())
                {
                    ImGui::Text("Radiance : %.10f", radiance);
                    ImGui::Text("Temperature : %.2f °C", radiance_to_temperature(radiance, products->get_wavenumber(select_image_id - 1)) - 273.15);
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
        ui_thread_pool.push([this](int)
                            { 
                    logger->info("Generating RGB Composite");
                    rgb_image = image::generate_composite_from_equ(images_obj, channel_numbers, rgb_equation, nlohmann::json(), &rgb_progress);
                    select_image_id = 0;
                    updateImage();
                    logger->info("Done");
                    rgb_processing = false; });
    }

    void ImageViewerHandler::drawMenu()
    {
        if (ImGui::CollapsingHeader("Image"))
        {
            if (ImGui::Combo("##imagechannelcombo", &select_image_id, select_image_str.c_str()))
                asyncUpdate();

            if (select_image_id > 0)
            {
                if (products->get_wavenumber(select_image_id - 1) != 0)
                {
                    ImGui::Text("Wavenumber : %f cm^-1", products->get_wavenumber(select_image_id - 1));
                    double wl_nm = 1e7 / products->get_wavenumber(select_image_id - 1);
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
                }
            }

            if (ImGui::Checkbox("Rotate", &rotate_image))
                asyncUpdate();

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

                auto result = pfd::save_file("Save Image", default_name, {"*.png"}, true);
                while (!result.ready(1000))
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));

                if (result.result().size() > 0)
                {
                    std::string path = result.result();
                    logger->info("Saving current image at {:s}", path.c_str());
                    current_image.save_png(path);
                }
            }
        }

        if (ImGui::CollapsingHeader("RGB Composites"))
        {
            if (ImGui::Combo("Preset", &select_rgb_presets, rgb_presets_str.c_str()))
            {
                rgb_equation = rgb_presets[select_rgb_presets].second;
                updateRGB();
            }

            ImGui::InputText("##rgbEquation", &rgb_equation);
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

        if (ImGui::CollapsingHeader("Products"))
        {
            if (ImGui::Button("Temperature"))
            {
                rgb_image = std::get<2>(products->images[select_image_id - 1]);
                rgb_image.to_rgb();

                for (size_t y = 0; y < std::get<2>(products->images[select_image_id - 1]).height(); y++)
                {
                    for (size_t x = 0; x < std::get<2>(products->images[select_image_id - 1]).width(); x++)
                    {
                        float temp_c = radiance_to_temperature(products->get_radiance_value(select_image_id - 1, x, y), products->get_wavenumber(select_image_id - 1)) - 273.15;

                        image::Image<uint16_t> lut = image::LUT_jet<uint16_t>();

                        int value = ((temp_c + 100) / 100) * 256;

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

    void ImageViewerHandler::drawContents(ImVec2 win_size)
    {
        image_view.draw(win_size);
    }
}