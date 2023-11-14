#include "markdown_helper.h"
#include "core/style.h"
#include "logger.h"

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

    MarkdownHelper::MarkdownHelper()
    {
    }

    void MarkdownHelper::init()
    {
        mdConfig.linkCallback = link_callback;
    };

    void MarkdownHelper::render()
    {
        mdConfig.headingFormats[0] = {style::bigFont, true};
        mdConfig.headingFormats[1] = {style::bigFont, true};
        mdConfig.headingFormats[2] = {style::baseFont, true};
        ImGui::Markdown(markdown_.c_str(), markdown_.length(), mdConfig);
    }

    void MarkdownHelper::set_md(std::string md)
    {
        markdown_ = md;
    }
}