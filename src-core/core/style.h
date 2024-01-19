#pragma once
#include "dll_export.h"
#include "imgui/imgui.h"
#include <string>

SATDUMP_DLL extern ImColor IMCOLOR_RED;
SATDUMP_DLL extern ImColor IMCOLOR_GREEN;
SATDUMP_DLL extern ImColor IMCOLOR_BLUE;
SATDUMP_DLL extern ImColor IMCOLOR_YELLOW;
SATDUMP_DLL extern ImColor IMCOLOR_ORANGE;
SATDUMP_DLL extern ImColor IMCOLOR_CYAN;
SATDUMP_DLL extern ImColor IMCOLOR_MAGENTA;
SATDUMP_DLL extern ImColor IMCOLOR_WINDOWBG;
SATDUMP_DLL extern ImColor IMCOLOR_OVERLAYBG;

namespace style
{
    SATDUMP_DLL extern ImFont *baseFont;
    SATDUMP_DLL extern ImFont *bigFont;
    //SATDUMP_DLL extern ImFont *hugeFont;

    bool setLightStyle();
    bool setDarkStyle();
    void beginDisabled();
    void endDisabled();
    void setFonts();
    void setFonts(float dpi_scaling);

    float macos_framebuffer_scale();
}