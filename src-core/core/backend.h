#pragma once
#include "dll_export.h"
#include <functional>
#include <stdint.h>
#include <string>
#include <utility>

namespace backend
{
    SATDUMP_DLL extern float device_scale;

    SATDUMP_DLL extern std::function<void()> rebuildFonts;
    SATDUMP_DLL extern std::function<void(int, int)> setMousePos;
    SATDUMP_DLL extern std::function<std::pair<int, int>()> beginFrame;
    SATDUMP_DLL extern std::function<void()> endFrame;
    SATDUMP_DLL extern std::function<void(uint8_t *, int, int)> setIcon;

    SATDUMP_DLL extern std::function<std::string(std::string)> selectFolderDialog;
    SATDUMP_DLL extern std::function<std::string(std::vector<std::pair<std::string, std::string>>, std::string)> selectFileDialog;
    SATDUMP_DLL extern std::function<std::string(std::vector<std::pair<std::string, std::string>>, std::string, std::string)> saveFileDialog;
} // namespace backend
