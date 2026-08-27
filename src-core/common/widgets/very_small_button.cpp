#include "very_small_button.h"
#include "imgui/imgui_internal.h"

namespace satdump
{
    namespace widgets
    {
        // Small buttons fits within text without additional vertical spacing.
        bool VerySmallButton(const char *label)
        {
            ImGuiContext &g = *ImGui::GetCurrentContext();
            float backup_padding_y = g.Style.FramePadding.y;
            float backup_padding_x = g.Style.FramePadding.x;
            g.Style.FramePadding.y = 0.0f;
            g.Style.FramePadding.x = 0.0f;
            bool pressed = ImGui::ButtonEx(label, ImVec2(0, 0), ImGuiButtonFlags_AlignTextBaseLine);
            g.Style.FramePadding.y = backup_padding_y;
            g.Style.FramePadding.x = backup_padding_x;
            return pressed;
        }
    } // namespace widgets
} // namespace satdump