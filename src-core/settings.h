#pragma once

#include <string>
#include "nlohmann/json.hpp"
#include "dll_export.h"

SATDUMP_DLL extern nlohmann::json settings;   // User settings / data, for example in the UI interface
SATDUMP_DLL extern nlohmann::json global_cfg; // Satdump-wise settings

void loadConfig();
void loadSettings(std::string path);
void saveSettings();