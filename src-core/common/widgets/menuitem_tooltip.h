#pragma once

/**
 * @file menuitem_tooltip.h
 */

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
        bool MenuItemTooltip(const char *label, const char *tooltip, const char *shortcut = __null, bool selected = false, bool enabled = true);
    } // namespace widgets
} // namespace satdump