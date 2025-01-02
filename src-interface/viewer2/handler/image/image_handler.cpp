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

                if (needs_to_be_disabled)
                    style::endDisabled();

                if (needs_to_update)
                    asyncProcess();
            }

            if (imgview_needs_update)
            {
                image_view.update(get_current_img());
                imgview_needs_update = false;
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
        }

        nlohmann::json ImageHandler::getConfig()
        {
            nlohmann::json p;
            p["equalize"] = equalize_img;
            p["individual_equalize"] = equalize_perchannel_img;
            p["white_balance"] = white_balance_img;
            p["normalize"] = normalize_img;
            p["median_blur"] = median_blur_img;
            return p;
        }

        void ImageHandler::do_process()
        {
            bool image_needs_processing = equalize_img |
                                          equalize_perchannel_img |
                                          white_balance_img |
                                          normalize_img |
                                          median_blur_img;

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
            }
            else
                curr_image.clear();

            has_second_image = image_needs_processing;
            imgview_needs_update = true;
        }
    }
}
