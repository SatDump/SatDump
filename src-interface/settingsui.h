#pragma once

#include <string>

extern bool use_light_theme;
extern float manual_dpi_scaling;
extern bool live_use_generated_output_folder;
extern std::string default_live_output_folder;
extern std::string default_recorder_output_folder;

void parseSettingsOrDefaults();
void renderSettings(int wwidth, int wheight);
