#include "settingsui.h"
#include "imgui/imgui.h"
#include "settings.h"
#include "module.h"
#include "logger.h"
#include "tle.h"
#include "imgui/file_selection.h"

bool use_light_theme;
float manual_dpi_scaling;
bool live_use_generated_output_folder;
std::string default_live_output_folder;
std::string default_recorder_output_folder;

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

    if (settings.contains("live_use_generated_output_folder"))
        live_use_generated_output_folder = settings["live_use_generated_output_folder"].get<int>();
    else
        live_use_generated_output_folder = false;

    if (settings.contains("default_live_output_folder"))
        default_live_output_folder = settings["default_live_output_folder"].get<std::string>();
    else
        default_live_output_folder = ".";

    if (settings.contains("default_recorder_output_folder"))
        default_recorder_output_folder = settings["default_recorder_output_folder"].get<std::string>();
    else
        default_recorder_output_folder = ".";
}

void renderSettings(int /*wwidth*/, int /*wheight*/)
{
    ImGui::Checkbox("Use Light Theme", &use_light_theme);

    ImGui::SetNextItemWidth(200 * ui_scale);
    ImGui::InputInt("Demod constellation size", &demod_constellation_size);

    ImGui::SetNextItemWidth(200 * ui_scale);
    ImGui::InputFloat("Manual DPI Scaling", &manual_dpi_scaling, 0.1f);

    ImGui::Checkbox("Auto-Generate live output folders", &live_use_generated_output_folder);

    if (ImGui::Button("Select Live Directory"))
    {
        logger->debug("Opening file dialog");

        std::string dir = selectDirectoryDialog("Select output directory", ".");

        if (dir.size() > 0)
            default_live_output_folder = dir;

        logger->debug("Dir " + default_live_output_folder);
    }
    ImGui::SameLine();
    ImGui::Text("Default live output directory : %s/", default_live_output_folder.c_str());

    if (ImGui::Button("Select Recorder Directory"))
    {
        logger->debug("Opening file dialog");

        std::string dir = selectDirectoryDialog("Select output directory", ".");

        if (dir.size() > 0)
            default_recorder_output_folder = dir;

        logger->debug("Dir " + default_recorder_output_folder);
    }
    ImGui::SameLine();
    ImGui::Text("Default recorder output directory : %s/", default_recorder_output_folder.c_str());

#ifndef __ANDROID__
    if (ImGui::Button("Update TLEs"))
        tle::updateTLEsMT();
#endif

    ImGui::BeginGroup();
    if (ImGui::Button("Save"))
    {
        settings["use_light_theme"] = use_light_theme;
        settings["demod_constellation_size"] = demod_constellation_size;
        settings["manual_dpi_scaling"] = manual_dpi_scaling;
        settings["live_use_generated_output_folder"] = live_use_generated_output_folder;
        settings["default_live_output_folder"] = default_live_output_folder;
        settings["default_recorder_output_folder"] = default_recorder_output_folder;

        saveSettings();
    }
    ImGui::SameLine();
    ImGui::TextColored(ImColor::HSV(0 / 360.0, 1, 1, 1.0), "Restarting SatDump may be necessary to apply some settings properly!");
    ImGui::EndGroup();
}