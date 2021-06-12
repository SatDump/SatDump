#include "settingsui.h"
#include "imgui/imgui.h"
#include "settings.h"

void renderSettings(int wwidth, int wheight)
{
    ImGui::Text("To be implemented soon...");
    ImGui::BeginGroup();
    if (ImGui::Button("Save"))
        saveSettings();
    ImGui::SameLine();
    ImGui::TextColored(ImColor::HSV(0 / 360.0, 1, 1, 1.0), "Restarting SatDump may be necessary to apply some settings");
    ImGui::EndGroup();
}