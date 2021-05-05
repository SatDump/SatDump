#pragma once

#include <string>
#include "nlohmann/json.hpp"
#include "dll_export.h"

SATDUMP_DLL extern nlohmann::json settings;

void loadSettings(std::string path);
void saveSettings();