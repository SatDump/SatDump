#define SATDUMP_DLL_EXPORT 1

#include "style.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
//#include <config.h>
//#include <options.h>
#include "logger.h"
#include <filesystem>
#include "core/module.h"
#include "resources.h"

#ifdef __APPLE__
#include <CoreGraphics/CGDirectDisplay.h>
#endif

SATDUMP_DLL ImColor IMCOLOR_RED;
SATDUMP_DLL ImColor IMCOLOR_GREEN;
SATDUMP_DLL ImColor IMCOLOR_BLUE;
SATDUMP_DLL ImColor IMCOLOR_YELLOW;
SATDUMP_DLL ImColor IMCOLOR_ORANGE;
SATDUMP_DLL ImColor IMCOLOR_CYAN;
SATDUMP_DLL ImColor IMCOLOR_MAGENTA;
SATDUMP_DLL ImColor IMCOLOR_WINDOWBG;
SATDUMP_DLL ImColor IMCOLOR_OVERLAYBG;

namespace style
{
    SATDUMP_DLL ImFont *baseFont;
    SATDUMP_DLL ImFont *bigFont;
    //SATDUMP_DLL ImFont *hugeFont;

    bool setLightStyle()
    {
        auto& style = ImGui::GetStyle();
        float round = pow(2.0f, ui_scale);
        style.WindowRounding = round;
        style.ChildRounding = round;
        style.FrameRounding = round;
        style.GrabRounding = round;
        style.PopupRounding = round;
        style.ScrollbarRounding = round;

        ImGui::StyleColorsLight();

        ImVec4 *colors = style.Colors;
        colors[ImGuiCol_TextDisabled] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);

        IMCOLOR_RED = ImColor(0.75f, 0.00f, 0.00f, 1.00f);
        IMCOLOR_GREEN = ImColor(0.06f, 0.50f, 0.00f, 1.00f);
        IMCOLOR_BLUE = ImColor(0.00f, 0.00f, 0.50f, 1.00f);
        IMCOLOR_YELLOW = ImColor(0.55f, 0.27f, 0.07f, 1.00f);
        IMCOLOR_ORANGE = ImColor(0.75f, 0.50f, 0.05f, 1.00f);
        IMCOLOR_CYAN = ImColor(0.00f, 0.50f, 0.50f, 1.00f);
        IMCOLOR_MAGENTA = ImColor(0.50f, 0.00f, 0.50f, 1.00f);
        IMCOLOR_WINDOWBG = ImColor(0.90f, 0.90f, 0.90f, 1.0f);
        IMCOLOR_OVERLAYBG = ImColor(1.00f, 1.00f, 1.00f, 0.71f);

        return true;
    }

    bool setDarkStyle()
    {
        auto& style = ImGui::GetStyle();
        float round = pow(2.0f, ui_scale);
        style.WindowRounding = round;
        style.ChildRounding = round;
        style.FrameRounding = round;
        style.GrabRounding = round;
        style.PopupRounding = round;
        style.ScrollbarRounding = round;

        ImGui::StyleColorsDark();

        ImVec4 *colors = style.Colors;
        colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
        colors[ImGuiCol_ChildBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
        colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.21f, 0.22f, 0.54f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.21f, 0.22f, 0.54f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.21f, 0.22f, 0.54f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.29f, 0.29f, 0.29f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_Button] = ImVec4(0.44f, 0.44f, 0.44f, 0.40f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.44f, 0.44f, 0.44f, 0.45f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.44f, 0.44f, 0.44f, 0.40f);
        colors[ImGuiCol_Header] = ImVec4(0.63f, 0.63f, 0.70f, 0.31f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.63f, 0.63f, 0.70f, 0.40f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.63f, 0.63f, 0.70f, 0.31f);
        colors[ImGuiCol_Separator] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.72f, 0.72f, 0.72f, 0.78f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.91f, 0.91f, 0.91f, 0.25f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.81f, 0.81f, 0.81f, 0.67f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.46f, 0.46f, 0.46f, 0.95f);
        colors[ImGuiCol_PlotLines] = ImVec4(0.4f, 0.9f, 1.0f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
        colors[ImGuiCol_PlotHistogram] = ImVec4(0.73f, 0.60f, 0.15f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.87f, 0.87f, 0.87f, 0.35f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
        colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
        colors[ImGuiCol_NavHighlight] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);

        IMCOLOR_RED = ImColor(1.00f, 0.00f, 0.00f, 1.00f);
        IMCOLOR_GREEN = ImColor(0.12f, 1.00f, 0.00f, 1.00f);
        IMCOLOR_BLUE = ImColor(0.00f, 0.00f, 1.00f, 1.00f);
        IMCOLOR_YELLOW = ImColor(1.00f, 1.00f, 0.00f, 1.00f);
        IMCOLOR_ORANGE = ImColor(1.00f, 0.67f, 0.00f, 0.07f);
        IMCOLOR_CYAN = ImColor(0.00f, 1.00f, 1.00f, 1.00f);
        IMCOLOR_MAGENTA = ImColor(1.00f, 0.00f, 1.00f, 1.00f);
        IMCOLOR_WINDOWBG = ImColor(0.0666f, 0.0666f, 0.0666f, 1.0f);
        IMCOLOR_OVERLAYBG = ImColor(0.00f, 0.00f, 0.00f, 0.71f);

        return true;
    }

    void beginDisabled()
    {
        ImVec4 *colors = ImGui::GetStyle().Colors;
        ImVec4 frame_bg = colors[ImGuiCol_FrameBg];
        ImVec4 button = colors[ImGuiCol_Button];
        frame_bg.w = 0.30f;
        button.w = 0.15f;

        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleColor(ImGuiCol_Button, button);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, frame_bg);
        ImGui::PushStyleColor(ImGuiCol_Text, colors[ImGuiCol_TextDisabled]);
    }

    void endDisabled()
    {
        ImGui::PopItemFlag();
        ImGui::PopStyleColor(3);
    }

    void setFonts()
    {
        setFonts(ui_scale);
    }

    void setFonts(float dpi_scaling)
    {
        ImGuiIO &io = ImGui::GetIO();
        (void)io;
        io.Fonts->Clear();
        const ImWchar def[] = {0x20, 0x2300, 0}; //default range
        const ImWchar list[6][3] = { {0xf000, 0xf0ff, 0}, {0xf400, 0xf4ff, 0}, {0xf800, 0xf8ff, 0},
            {0xfc00, 0xfcff, 0}, {0xea00, 0xeaff, 0}, {0xf200, 0xf2ff, 0} };
        static ImFontConfig config;
        float macos_fbs = macos_framebuffer_scale();
        float font_scaling = dpi_scaling * macos_fbs;

        baseFont = io.Fonts->AddFontFromFileTTF(resources::getResourcePath("fonts/Roboto-Medium.ttf").c_str(), 16.0f * font_scaling, &config, def);
        config.MergeMode = true;

        for (int i = 0; i < 6; i++)
            baseFont = io.Fonts->AddFontFromFileTTF(resources::getResourcePath("fonts/font.ttf").c_str(), 16.0f * font_scaling, &config, list[i]);
        config.MergeMode = false;

        bigFont = io.Fonts->AddFontFromFileTTF(resources::getResourcePath("fonts/Roboto-Medium.ttf").c_str(), 45.0f * font_scaling);   //, &config, ranges);
        //hugeFont = io.Fonts->AddFontFromFileTTF(resources::getResourcePath("fonts/Roboto-Medium.ttf").c_str(), 128.0f * font_scaling); //, &config, ranges);
        io.Fonts->Build();
        io.FontGlobalScale = 1 / macos_fbs;
    }

    float macos_framebuffer_scale()
    {
#ifdef __APPLE__
        CGDirectDisplayID display_id = CGMainDisplayID();
        CGDisplayModeRef display_mode = CGDisplayCopyDisplayMode(display_id);
        float return_value = (float)CGDisplayModeGetPixelWidth(display_mode) / (float)CGDisplayPixelsWide(display_id);
        CGDisplayModeRelease(display_mode);
        return return_value;
#else
        return 1.0f;
#endif
    }
}