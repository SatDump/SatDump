#include "markdown_helper.h"
#include "core/style.h"

namespace widgets
{
    void MarkdownHelper::link_callback(ImGui::MarkdownLinkCallbackData data_)
    {
        std::string url(data_.link, data_.linkLength);
        if (!data_.isImage)
        {
            system(std::string("xdg-open " + url).c_str());
        }
    }

    MarkdownHelper::MarkdownHelper()
    {
    }

    void MarkdownHelper::init()
    {
        mdConfig.linkCallback = link_callback;

        mdConfig.headingFormats[0] = {style::bigFont, true};
        mdConfig.headingFormats[1] = {style::bigFont, true};
        mdConfig.headingFormats[2] = {style::baseFont, true};
    };

    void MarkdownHelper::render()
    {
        ImGui::Markdown(markdown_.c_str(), markdown_.length(), mdConfig);
    }

    void MarkdownHelper::set_md(std::string md)
    {
        markdown_ = md;
    }
}