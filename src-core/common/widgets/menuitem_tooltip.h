#pragma once

/**
 * @file menuitem_tooltip.h
 */

#include <cstddef>

namespace satdump
{
    namespace widgets
    {
        /**
         * @brief Just an ImGui::MenuItem, but with a built-in tooltip when hovered
         * @param label See ImGui
         * @param tooltip Tooltip string
         * @param shortcut See ImGui
         * @param selected See ImGui
         * @param enabled See ImGui
         */
        bool MenuItemTooltip(const char *label, const char *tooltip, const char *shortcut = NULL, bool selected = false, bool enabled = true);

        /**
         * @brief Just an ImGui::MenuItem, but with a built-in tooltip when hovered
         * @param label See ImGui
         * @param tooltip Tooltip string
         * @param enabled See ImGui
         */
        bool BeginMenuTooltip(const char *label, const char *tooltip, bool enabled = true);
    } // namespace widgets
} // namespace satdump