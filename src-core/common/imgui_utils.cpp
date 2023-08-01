#include "imgui_utils.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

void ImGuiUtils_BringCurrentWindowToFront() 
{
    ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());
}