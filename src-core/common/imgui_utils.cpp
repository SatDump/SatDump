#include "imgui_utils.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

void ImGuiUtils_BringCurrentWindowToFront() 
{
    ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());
}

void ImGuiUtils_SendCurrentWindowToBack()
{
    ImGuiWindow* current_window = ImGui::GetCurrentWindow();
    if(ImGui::FindBottomMostVisibleWindowWithinBeginStack(current_window) != current_window)
        ImGui::BringWindowToDisplayBack(current_window);
}
