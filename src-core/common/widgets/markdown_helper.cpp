#include "markdown_helper.h"
#include "core/style.h"
#include "logger.h"
#include "imgui/imgui_image.h"
#include "resources.h"
#include "common/image/image.h"
#include <filesystem>

namespace widgets
{
    void MarkdownHelper::link_callback(ImGui::MarkdownLinkCallbackData data_)
    {
        std::string url(data_.link, data_.linkLength);
        if (!data_.isImage)
        {
            logger->info("Opening URL " + url);

#if defined(_WIN32)
            system(std::string("explorer \"" + url + "\"").c_str());
#elif defined(__APPLE__)
            system(std::string("open " + url).c_str());
#else
            system(std::string("xdg-open " + url).c_str());
#endif
        }
    }

    ImGui::MarkdownImageData MarkdownHelper::image_callback(ImGui::MarkdownLinkCallbackData data_)
    {
        MarkdownHelper *tthis = (MarkdownHelper *)data_.userData;

        std::string image_path(data_.link, data_.linkLength);

        ImGui::MarkdownImageData imageData;

        if (tthis->texture_buffer.count(image_path))
        {
            imageData = tthis->texture_buffer[image_path];
        }
        else
        {
            imageData.isValid = false;

            if (std::filesystem::exists(resources::getResourcePath(image_path)))
            {
                logger->trace("Loading image for markdown : " + image_path);

                image::Image<uint8_t> img;
                img.load_img(resources::getResourcePath(image_path));

                unsigned int text_id = makeImageTexture();
                uint32_t *output_buffer = new uint32_t[img.width() * img.height()];
                uchar_to_rgba(img.data(), output_buffer, img.width() * img.height(), img.channels());
                updateImageTexture(text_id, output_buffer, img.width(), img.height());
                delete[] output_buffer;

                imageData.isValid = true;
                imageData.useLinkCallback = false;
                imageData.user_texture_id = (ImTextureID)(intptr_t)text_id;
                imageData.size = ImVec2(img.width(), img.height());
            }

            tthis->texture_buffer.emplace(image_path, imageData);
        }

        // For image resize when available size.x > image width, add
        ImVec2 const contentSize = ImGui::GetContentRegionAvail();
        if (imageData.size.x > contentSize.x)
        {
            float const ratio = imageData.size.y / imageData.size.x;
            imageData.size.x = contentSize.x;
            imageData.size.y = contentSize.x * ratio;
        }

        return imageData;
    }

    MarkdownHelper::MarkdownHelper()
    {
    }

    void MarkdownHelper::init()
    {
    }

    void MarkdownHelper::render()
    {
        mdConfig.linkCallback = link_callback;
        mdConfig.imageCallback = image_callback;
        mdConfig.headingFormats[0] = {style::bigFont, true};
        mdConfig.headingFormats[1] = {style::bigFont, true};
        mdConfig.headingFormats[2] = {style::baseFont, true};
        mdConfig.userData = (void *)this;
        ImGui::Markdown(markdown_.c_str(), markdown_.length(), mdConfig);
    }

    void MarkdownHelper::set_md(std::string md)
    {
        markdown_ = md;
        texture_buffer.clear();
    }
}