#pragma once
#include "dll_export.h"
#include "imgui/imgui.h"
#include <string>

namespace style
{
    SATDUMP_DLL extern ImFont *baseFont;
    SATDUMP_DLL extern ImFont *bigFont;
    //SATDUMP_DLL extern ImFont *hugeFont;

    bool setDefaultStyle();
    bool setLightStyle();
    bool setDarkStyle();
    void beginDisabled();
    void endDisabled();
    void setFonts();
    void setFonts(float dpi_scaling);

    float macos_framebuffer_scale();
}