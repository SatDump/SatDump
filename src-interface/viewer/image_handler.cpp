#include "image_handler.h"

namespace satdump
{
    void ImageViewerHandler::init()
    {
        products = (ImageProducts *)ViewerHandler::products;

        // TMP
        rgb_presets.push_back({"221", "ch2, ch2, ch1"});
        rgb_presets.push_back({"321", "ch3, ch2, ch1"});
        rgb_presets.push_back({"321/221 Mix", "(ch3 * 0.4 + ch2 * 0.6) * 2.2 - 0.15, ch2 * 2.2 - 0.15, ch1 * 2.2 - 0.15"});

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

        if (invert_image)
            current_image.linear_invert();

        image_view.update(current_image);
        // current_image.clear();

        // Tooltip function
        image_view.mouseCallback = [this](int x, int y)
        {
            ImGui::BeginTooltip();
            ImGui::Text("Raw (16-bits) : %d", current_image[y * current_image.width() + x]);
            ImGui::Text("Temperature : %.2fC", current_image[y * current_image.width() + x] / 2000.0f);
            ImGui::EndTooltip();

            /*if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                logger->info("{:d}, {:d}", x, y);
            }*/
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

            if (ImGui::Checkbox("Rotate", &rotate_image))
                asyncUpdate();

            if (ImGui::Checkbox("Equalize", &equalize_image))
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
    }

    void ImageViewerHandler::drawContents(ImVec2 win_size)
    {
        image_view.draw(win_size);
    }
}