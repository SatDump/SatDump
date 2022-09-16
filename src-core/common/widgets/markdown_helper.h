#pragma once

#include <string>
#include "imgui/imgui.h"
#include "imgui/imgui_markdown.h"

namespace widgets
{
    class MarkdownHelper
    {
    private:
        std::string markdown_;
        ImGui::MarkdownConfig mdConfig;

        static void link_callback(ImGui::MarkdownLinkCallbackData data_);

    public:
        MarkdownHelper();
        void init();
        void render();
        void set_md(std::string md);
    };
}