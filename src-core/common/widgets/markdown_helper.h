#pragma once

#include "imgui/imgui.h"
#include "imgui/imgui_markdown.h"
#include <map>
#include <string>

namespace satdump
{
    namespace widgets
    {
        class MarkdownHelper
        {
        private:
            std::string markdown_;
            ImGui::MarkdownConfig mdConfig;

            static void link_callback(ImGui::MarkdownLinkCallbackData data_);
            static ImGui::MarkdownImageData image_callback(ImGui::MarkdownLinkCallbackData data_);

            std::map<std::string, ImGui::MarkdownImageData> texture_buffer;

        public:
            MarkdownHelper();
            void render();
            void set_md(std::string md);
        };
    } // namespace widgets
} // namespace satdump