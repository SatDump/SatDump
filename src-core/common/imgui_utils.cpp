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

bool ImGuiUtils_OfflineProcessingSelected()
{
    for (int i = 0; i < GImGui->TabBars.GetMapSize(); i++)
        if (ImGuiTabBar* tab_bar = GImGui->TabBars.TryGetMapData(i))
            if (tab_bar->SelectedTabId != 0 &&
                strcmp(ImGui::TabBarGetTabName(tab_bar, ImGui::TabBarFindTabByID(tab_bar, tab_bar->SelectedTabId)), "Offline processing") == 0)
                    return true;
    return false;
}