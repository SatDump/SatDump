#include "menuitem_tooltip.h"
#include "imgui/imgui.h"

namespace satdump
{
    namespace widgets
    {
        bool MenuItemTooltip(const char *label, const char *tooltip, const char *shortcut, bool selected, bool enabled)
        {
            bool v = ImGui::MenuItem(label, shortcut, selected, enabled);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip(tooltip);
            return v;
        }
    } // namespace widgets
} // namespace satdump