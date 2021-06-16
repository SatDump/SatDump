#include "settingsui.h"
#include "imgui/imgui.h"
#include "settings.h"

bool use_light_theme;

void parseSettingsOrDefaults()
{
    if (settings.contains("use_light_theme"))
        use_light_theme = settings["use_light_theme"].get<int>();
    else
        use_light_theme = false;
}

void renderSettings(int wwidth, int wheight)
{
    ImGui::Checkbox("Use Light Theme", &use_light_theme);

    ImGui::BeginGroup();
    if (ImGui::Button("Save"))
    {
        settings["use_light_theme"] = use_light_theme;

        saveSettings();
    }
    ImGui::SameLine();
    ImGui::TextColored(ImColor::HSV(0 / 360.0, 1, 1, 1.0), "Restarting SatDump may be necessary to apply some settings properly!");
    ImGui::EndGroup();
}