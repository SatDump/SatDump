#include "image_handler.h"

#include "core/config.h"
#include "common/calibration.h"

#include "imgui/imgui_internal.h"

namespace satdump
{
    void ImageViewerHandler::init()
    {
        products = (ImageProducts *)ViewerHandler::products;

        // TMP
        if (instrument_cfg.contains("rgb_composites"))
            for (nlohmann::detail::iteration_proxy_value<nlohmann::detail::iter_impl<nlohmann::ordered_json>> compo : instrument_cfg["rgb_composites"].items())
                rgb_presets.push_back({compo.key(), compo.value().get<ImageCompositeCfg>()});

        select_image_str += std::string("Composite") + '\0';

        for (int i = 0; i < products->images.size(); i++)
        {
            auto img = products->images[i];
            select_image_str += "Channel " + img.channel_name + '\0';
            channel_numbers.push_back(img.channel_name);
            images_obj.push_back(img.image);
        }

        for (std::pair<std::string, ImageCompositeCfg> &compo : rgb_presets)
            rgb_presets_str += compo.first + '\0';

        asyncUpdate();
    }

    void ImageViewerHandler::updateImage()
    {
        if (select_image_id == 0)
            current_image = rgb_image;
        else
            current_image = products->images[select_image_id - 1].image;

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
            if (active_channel_id >= 0)
            {
                if (rotate_image)
                {
                    x = current_image.width() - 1 - x;
                    y = current_image.height() - 1 - y;
                }

                int raw_value = products->images[active_channel_id].image[y * current_image.width() + x] >> (16 - products->bit_depth);
                double radiance = products->get_radiance_value(active_channel_id, x, y);

                ImGui::BeginTooltip();
                ImGui::Text("Count : %d", raw_value);
                if (products->has_calibation())
                {
                    ImGui::Text("Radiance : %.10f", radiance);
                    ImGui::Text("Temperature : %.2f °C", radiance_to_temperature(radiance, products->get_wavenumber(active_channel_id)) - 273.15);
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
                    logger->info("Generating RGB Composite");
                    satdump::ImageCompositeCfg cfg;
                    cfg.equation = rgb_compo_cfg.equation;
                    cfg.offsets = rgb_compo_cfg.offsets;

                    equalize_image = rgb_compo_cfg.equalize;
                    invert_image = rgb_compo_cfg.invert;
                    normalize_image = rgb_compo_cfg.normalize;
                    white_balance_image = rgb_compo_cfg.white_balance;

                    rgb_image = satdump::make_composite_from_product(*products, cfg, &rgb_progress);//image::generate_composite_from_equ(images_obj, channel_numbers, rgb_equation, nlohmann::json(), &rgb_progress);
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
            {
                if (select_image_id == 0)
                    active_channel_id = -1;
                else
                    active_channel_id = select_image_id - 1;
                asyncUpdate();
            }

            if (active_channel_id >= 0)
            {
                if (products->get_wavenumber(active_channel_id) != 0)
                {
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
                            float temp_c = radiance_to_temperature(products->get_radiance_value(active_channel_id, x, y), products->get_wavenumber(active_channel_id)) - 273.15;

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
    }

    void ImageViewerHandler::drawContents(ImVec2 win_size)
    {
        image_view.draw(win_size);
    }

    // returns the node's rectangle
    /*ImRect RenderTree(Node *n)
    {
        const bool recurse = ImGui::TreeNode(...);
        const ImRect nodeRect = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());

        if (recurse)
        {
            const ImColor TreeLineColor = ImGui::GetColorU32(ImGuiCol_Text);
            const float SmallOffsetX = 11.0f; // for now, a hardcoded value; should take into account tree indent size
            ImDrawList *drawList = ImGui::GetWindowDrawList();

            ImVec2 verticalLineStart = ImGui::GetCursorScreenPos();
            verticalLineStart.x += SmallOffsetX; // to nicely line up with the arrow symbol
            ImVec2 verticalLineEnd = verticalLineStart;

            for (Node *child : *n)
            {
                const float HorizontalTreeLineSize = 8.0f; // chosen arbitrarily
                const ImRect childRect = RenderTree(child);
                const float midpoint = (childRect.Min.y + childRect.Max.y) / 2.0f;
                drawList->AddLine(ImVec2(verticalLineStart.x, midpoint), ImVec(verticalLineStart.x + HorizontalTreeLineSize, midpoint), TreeLineColor);
                verticalLineEnd.y = midpoint;
            }

            drawList->AddLine(verticalLineStart, verticalLineEnd, TreeLineColor);
        }

        return nodeRect;
    }*/

    float ImageViewerHandler::drawTreeMenu()
    {
        const ImColor TreeLineColor = ImColor(128, 128, 128, 255); // ImGui::GetColorU32(ImGuiCol_Text);
        const float SmallOffsetX = 11.0f;                          // for now, a hardcoded value; should take into account tree indent size
        ImDrawList *drawList = ImGui::GetWindowDrawList();

        ImVec2 verticalLineStart = ImGui::GetCursorScreenPos();
        verticalLineStart.x += SmallOffsetX; // to nicely line up with the arrow symbol
        ImVec2 verticalLineEnd = verticalLineStart;

        ImGui::TreeNodeEx("Channel 1 equ", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
        if (ImGui::IsItemClicked())
        {
            select_image_id = 1;
            active_channel_id = 0;
            equalize_image = true;
            asyncUpdate();
        }
        {
            const float HorizontalTreeLineSize = 8.0f;                                         // chosen arbitrarily
            const ImRect childRect = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax()); // RenderTree(child);
            const float midpoint = (childRect.Min.y + childRect.Max.y) / 2.0f;
            drawList->AddLine(ImVec2(verticalLineStart.x, midpoint), ImVec2(verticalLineStart.x + HorizontalTreeLineSize, midpoint), TreeLineColor);
            verticalLineEnd.y = midpoint;
        }

        ImGui::TreeNodeEx("Channel 2 White Balance", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
        if (ImGui::IsItemClicked())
        {
            select_image_id = 2;
            active_channel_id = 1;
            equalize_image = false;
            white_balance_image = true;
            asyncUpdate();
        }
        {
            const float HorizontalTreeLineSize = 8.0f;                                         // chosen arbitrarily
            const ImRect childRect = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax()); // RenderTree(child);
            const float midpoint = (childRect.Min.y + childRect.Max.y) / 2.0f;
            drawList->AddLine(ImVec2(verticalLineStart.x, midpoint), ImVec2(verticalLineStart.x + HorizontalTreeLineSize, midpoint), TreeLineColor);
            verticalLineEnd.y = midpoint;
        }

        drawList->AddLine(verticalLineStart, verticalLineEnd, TreeLineColor);

        return verticalLineEnd.y - verticalLineStart.y;
    }
}