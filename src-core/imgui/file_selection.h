#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include "dll_export.h"

SATDUMP_DLL extern std::function<std::string(std::string, std::string, std::vector<std::string>)> selectInputFileDialog;
SATDUMP_DLL extern std::function<std::string(std::string, std::string)> selectDirectoryDialog;
SATDUMP_DLL extern std::function<std::string(std::string, std::string, std::vector<std::string>)> selectOutputFileDialog;
