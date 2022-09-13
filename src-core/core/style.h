#pragma once

#include "imgui/imgui.h"
#include <string>

namespace style
{
    extern ImFont *baseFont;
    extern ImFont *bigFont;
    extern ImFont *hugeFont;

    bool setDefaultStyle();
    bool setLightStyle(float dpi_scaling = 1.0f);
    bool setDarkStyle(float dpi_scaling = 1.0f);
    void beginDisabled();
    void endDisabled();
    void setFonts();
}