#pragma once
#include "dll_export.h"
#include "imgui/imgui.h"
#include <string>

namespace style
{
    SATDUMP_DLL extern ImFont *baseFont;
    SATDUMP_DLL extern ImFont *bigFont;
    SATDUMP_DLL extern ImFont *hugeFont;

    bool setDefaultStyle();
    bool setLightStyle(float dpi_scaling = 1.0f);
    bool setDarkStyle(float dpi_scaling = 1.0f);
    void beginDisabled();
    void endDisabled();
    void setFonts();
}