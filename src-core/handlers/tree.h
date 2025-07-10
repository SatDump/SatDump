#pragma once

/**
 * @file tree.h
 */

#include "core/style.h"

namespace satdump
{
    namespace handlers
    {
        /**
         * @brief Handler Tree Drawer
         *
         * Draws the left-panel explorer tree lines.
         */
        struct TreeDrawer
        {
            float SmallOffsetX = 11.0f - 22; // for now, a hardcoded value; should take into account tree indent size

            ImDrawList *drawList = nullptr;

            ImVec2 verticalLineStart = {0, 0};

            ImVec2 verticalLineEnd = {0, 0};

            void start()
            {
                drawList = ImGui::GetWindowDrawList();

                verticalLineStart = ImGui::GetCursorScreenPos();
                verticalLineStart.x += SmallOffsetX * ui_scale; // to nicely line up with the arrow symbol
                verticalLineEnd = verticalLineStart;
            }

            bool node(std::string icon)
            {
                const float HorizontalTreeLineSize = 8.0f * ui_scale; // chosen arbitrarily
                const float minY = ImGui::GetCursorScreenPos().y - 20 * ui_scale;
                const float midpoint = minY + HorizontalTreeLineSize;
                drawList->AddLine(ImVec2(verticalLineStart.x, midpoint), ImVec2(verticalLineStart.x + HorizontalTreeLineSize, midpoint), style::theme.treeview_icon);
                drawList->AddText(ImVec2(verticalLineStart.x + HorizontalTreeLineSize * 2.0f, minY), style::theme.treeview_icon, icon.c_str());
                verticalLineEnd.y = midpoint;
                return false;
            }

            float end()
            {
                drawList->AddLine(verticalLineStart, verticalLineEnd, style::theme.treeview_icon);

                return verticalLineEnd.y - verticalLineStart.y;
            }
        };
    } // namespace handlers
} // namespace satdump