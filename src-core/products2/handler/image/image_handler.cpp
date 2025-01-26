#include "image_handler.h"

#include "imgui/imgui_stdlib.h"
#include "core/style.h"

#include "common/image/io.h" // TODOREWORK
#include "common/image/processing.h"

#include "nlohmann/json_utils.h"

namespace satdump
{
    namespace viewer
    {
        ImageHandler::ImageHandler()
        {
        }

        ImageHandler::~ImageHandler()
        {
        }

        void ImageHandler::drawMenu()
        {
            bool needs_to_be_disabled = is_processing;

            if (ImGui::CollapsingHeader("Image"))
            {
                bool needs_to_update = false;

                if (needs_to_be_disabled)
                    style::beginDisabled();

                needs_to_update |= ImGui::Checkbox("Equalize", &equalize_img);
                needs_to_update |= ImGui::Checkbox("Individual Equalize", &equalize_perchannel_img);
                needs_to_update |= ImGui::Checkbox("White Balance", &white_balance_img);
                needs_to_update |= ImGui::Checkbox("Normalize", &normalize_img);
                needs_to_update |= ImGui::Checkbox("Median Blur", &median_blur_img);
                needs_to_update |= ImGui::Checkbox("Rotate 180", &rotate180_image);

                if (needs_to_be_disabled)
                    style::endDisabled();

                if (image_calib_valid)
                {
                    ImGui::Text("Calibration Unit %s", image_calib.unit.c_str());
                    ImGui::Text("Calibration Min %f", image_calib.min);
                    ImGui::Text("Calibration Max %f", image_calib.max);
                }

                if (needs_to_update)
                    asyncProcess();
            }

            if (imgview_needs_update)
            {
                image_view.update(get_current_img());
                imgview_needs_update = false;

                image_view.mouseCallback = [this](int x, int y)
                {
                    auto &img = get_current_img();
                    ImGui::BeginTooltip(); // TODOREWORK
                    additionalMouseCallback(x, y);
                    ImGui::Text("Raw : %d", img.get(0, x, y));
                    if (image_calib_valid && image.channels() == 1)
                    {
                        double val = image_calib.getVal(img.getf(0, x, y));
                        ImGui::Text("Unit : %f %s", val, image_calib.unit.c_str());
                    }
                    if (image_proj_valid)
                    {
                        geodetic::geodetic_coords_t pos;
                        if (image_proj.inverse(x, y, pos))
                        {
                            ImGui::Text("Lat : Invalid!");
                            ImGui::Text("Lon : Invalid!");
                        }
                        else
                        {
                            ImGui::Text("Lat : %f", pos.lat);
                            ImGui::Text("Lon : %f", pos.lon);
                        }
                    }
                    ImGui::EndTooltip();
                };
            }
        }

        void ImageHandler::drawMenuBar()
        {
            if (ImGui::MenuItem("Save Image"))
            {
                logger->warn("Saving image to /tmp/out.png!");
                image::save_img(get_current_img(), "/tmp/out.png");
            }
        }

        void ImageHandler::drawContents(ImVec2 win_size)
        {
            image_view.draw(win_size);
        }

        void ImageHandler::setConfig(nlohmann::json p)
        {
            equalize_img = getValueOrDefault(p["equalize"], false);
            equalize_perchannel_img = getValueOrDefault(p["individual_equalize"], false);
            white_balance_img = getValueOrDefault(p["white_balance"], false);
            normalize_img = getValueOrDefault(p["normalize"], false);
            median_blur_img = getValueOrDefault(p["median_blur"], false);
            rotate180_image = getValueOrDefault(p["rotate180_image"], false);
        }

        nlohmann::json ImageHandler::getConfig()
        {
            nlohmann::json p;
            p["equalize"] = equalize_img;
            p["individual_equalize"] = equalize_perchannel_img;
            p["white_balance"] = white_balance_img;
            p["normalize"] = normalize_img;
            p["median_blur"] = median_blur_img;
            p["rotate180_image"] = rotate180_image;
            return p;
        }

        void ImageHandler::updateImage(image::Image &img) // TODOREWORK
        {
            image::set_metadata(image, {});
            image = img;
            process();
        }

        void ImageHandler::do_process()
        {
            bool image_needs_processing = equalize_img |
                                          equalize_perchannel_img |
                                          white_balance_img |
                                          normalize_img |
                                          median_blur_img |
                                          rotate180_image;

            if (image_needs_processing)
            {
                curr_image = image;

                if (equalize_img)
                    image::equalize(curr_image, false);
                if (equalize_perchannel_img)
                    image::equalize(curr_image, true);
                if (white_balance_img)
                    image::white_balance(curr_image);
                if (normalize_img)
                    image::normalize(curr_image);
                if (median_blur_img)
                    image::median_blur(curr_image);
                if (rotate180_image)
                    curr_image.mirror(true, true);
            }
            else
                curr_image.clear();

            has_second_image = image_needs_processing;
            imgview_needs_update = true;

            image_proj_valid = false;
            if (image::has_metadata_proj_cfg(image))
            {
                image_proj = image::get_metadata_proj_cfg(image);
                image_proj.init(0, 1);
                image_proj_valid = true;
            }

            image_calib_valid = false;
            if (image::has_metadata_calib_cfg(image))
            {
                image_calib = image::get_metadata_calib_cfg(image);
                image_calib_valid = true;
            }
        }
    }
}
