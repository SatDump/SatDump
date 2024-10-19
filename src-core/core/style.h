#pragma once
#include "dll_export.h"
#include "imgui/imgui.h"
#include <string>

SATDUMP_DLL extern float ui_scale;               // UI Scaling factor, for DPI scaling
SATDUMP_DLL extern int demod_constellation_size; // Demodulator constellation size

namespace style
{
    struct Theme
    {
        std::string font = "Roboto-Medium";
        float font_size = 16.0f;

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
        ImColor plot_bg;
        ImColor fft_graduations;
        ImColor widget_bg;
        ImColor frame_bg;
        ImColor notification_bg;
        ImColor overlay_bg;
        ImColor freq_highlight;
    };

    SATDUMP_DLL extern Theme theme;
    SATDUMP_DLL extern ImFont *baseFont;
    SATDUMP_DLL extern ImFont *bigFont;
    //SATDUMP_DLL extern ImFont *hugeFont;

    void setStyle();
    void beginDisabled();
    void endDisabled();
    void setFonts(float dpi_scaling);

    float macos_framebuffer_scale();
}