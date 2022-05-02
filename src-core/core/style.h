#pragma once

#include "imgui/imgui.h"
#include <string>

namespace style
{
    extern ImFont *baseFont;
    extern ImFont *bigFont;
    extern ImFont *hugeFont;

    bool setDefaultStyle(std::string resDir);
    bool setLightStyle(std::string resDir, float dpi_scaling = 1.0f);
    bool setDarkStyle(std::string resDir, float dpi_scaling = 1.0f);
    void beginDisabled();
    void endDisabled();
}