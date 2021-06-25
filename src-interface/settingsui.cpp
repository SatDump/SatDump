#include "settingsui.h"
#include "imgui/imgui.h"
#include "settings.h"
#include "module.h"

bool use_light_theme;
float manual_dpi_scaling;

void parseSettingsOrDefaults()
{
    if (settings.contains("use_light_theme"))
        use_light_theme = settings["use_light_theme"].get<int>();
    else
        use_light_theme = false;

    if (settings.contains("demod_constellation_size"))
        demod_constellation_size = settings["demod_constellation_size"].get<int>();
    else
        demod_constellation_size = 200;

    if (settings.contains("manual_dpi_scaling"))
        manual_dpi_scaling = settings["manual_dpi_scaling"].get<float>();
    else
        manual_dpi_scaling = 1.0f;
}

void renderSettings(int wwidth, int wheight)
{
    ImGui::Checkbox("Use Light Theme", &use_light_theme);
    ImGui::SetNextItemWidth(200 * ui_scale);
    ImGui::InputInt("Demod constellation size", &demod_constellation_size);
    ImGui::SetNextItemWidth(200 * ui_scale);
    ImGui::InputFloat("Manual DPI Scaling", &manual_dpi_scaling, 0.1f);

    ImGui::BeginGroup();
    if (ImGui::Button("Save"))
    {
        settings["use_light_theme"] = use_light_theme;
        settings["demod_constellation_size"] = demod_constellation_size;
        settings["manual_dpi_scaling"] = manual_dpi_scaling;

        saveSettings();
    }
    ImGui::SameLine();
    ImGui::TextColored(ImColor::HSV(0 / 360.0, 1, 1, 1.0), "Restarting SatDump may be necessary to apply some settings properly!");
    ImGui::EndGroup();
}