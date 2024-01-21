#pragma once
#include "dll_export.h"
#include "imgui/imgui.h"
#include <string>

namespace style
{
    struct Theme
    {
        bool light_mode = false;

        ImColor red;
        ImColor green;
        ImColor blue;
        ImColor yellow;
        ImColor orange;
        ImColor cyan;
        ImColor fuchsia;
        ImColor magenta;
        ImColor lavender;
        ImColor light_green;
        ImColor light_cyan;

        ImColor constellation;
        ImColor frame_bg;
        ImColor overlay_bg;
        ImColor freq_highlight;
    };

    SATDUMP_DLL extern Theme theme;
    SATDUMP_DLL extern ImFont *baseFont;
    SATDUMP_DLL extern ImFont *bigFont;
    //SATDUMP_DLL extern ImFont *hugeFont;

    void setLightStyle();
    void setDarkStyle();
    void beginDisabled();
    void endDisabled();
    void setFonts();
    void setFonts(float dpi_scaling);

    float macos_framebuffer_scale();
}