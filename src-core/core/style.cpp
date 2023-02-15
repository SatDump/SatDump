#include "style.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
//#include <config.h>
//#include <options.h>
#include <spdlog/spdlog.h>
#include <filesystem>
#include "core/module.h"
#include "resources.h"

namespace style
{
    ImFont *baseFont;
    ImFont *bigFont;
    ImFont *hugeFont;

    bool setDefaultStyle()
    {
        float round = 2.0f;
        ImGui::GetStyle().WindowRounding = round;
        ImGui::GetStyle().ChildRounding = round;
        ImGui::GetStyle().FrameRounding = round;
        ImGui::GetStyle().GrabRounding = round;
        ImGui::GetStyle().PopupRounding = round;
        ImGui::GetStyle().ScrollbarRounding = round;

        setFonts();

        ImGui::StyleColorsDark();
        // ImGui::StyleColorsLight();

        return true;
    }

    bool setLightStyle(float dpi_scaling)
    {
        float round = 2.0f;
        ImGui::GetStyle().WindowRounding = round;
        ImGui::GetStyle().ChildRounding = round;
        ImGui::GetStyle().FrameRounding = round;
        ImGui::GetStyle().GrabRounding = round;
        ImGui::GetStyle().PopupRounding = round;
        ImGui::GetStyle().ScrollbarRounding = round;

        setFonts();

        ImGui::StyleColorsLight();

        /*auto &style = ImGui::GetStyle();

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
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);*/

        ImGui::GetStyle().ScaleAllSizes(dpi_scaling);
        ui_scale = dpi_scaling;

        return true;
    }

    bool setDarkStyle(float dpi_scaling)
    {
        float round = 2.0f;
        ImGui::GetStyle().WindowRounding = round;
        ImGui::GetStyle().ChildRounding = round;
        ImGui::GetStyle().FrameRounding = round;
        ImGui::GetStyle().GrabRounding = round;
        ImGui::GetStyle().PopupRounding = round;
        ImGui::GetStyle().ScrollbarRounding = round;

        setFonts();

        ImGui::StyleColorsDark();

        auto &style = ImGui::GetStyle();

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

        ImGui::GetStyle().ScaleAllSizes(dpi_scaling);
        ui_scale = dpi_scaling;

        return true;
    }

    void beginDisabled()
    {
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.44f, 0.44f, 0.44f, 0.15f));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.20f, 0.21f, 0.22f, 0.30f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.00f, 1.00f, 1.00f, 0.65f));
    }

    void endDisabled()
    {
        ImGui::PopItemFlag();
        ImGui::PopStyleColor(3);
    }

    void setFonts()
    {

        ImFontGlyphRangesBuilder builder;
        static const ImWchar def[] = {0x20, 0x2300, 0}; //default range
        static ImFontConfig config;
        baseFont = ImGui::GetIO().Fonts->AddFontFromFileTTF(resources::getResourcePath("fonts/font.ttf").c_str(), 16.0f * ui_scale, &config, def);
        config.MergeMode = true;
        static const ImWchar list[6][3] = {{0xf000, 0xf0ff, 0}, {0xf400, 0xf4ff, 0}, {0xf800, 0xf8ff, 0}, {0xfc00, 0xfcff, 0}, {0xea00, 0xeaff, 0}, {0xf200, 0xf2ff, 0}};   

        for (int i = 0; i < 6; i++)
            baseFont = ImGui::GetIO().Fonts->AddFontFromFileTTF(resources::getResourcePath("fonts/font.ttf").c_str(), 16.0f * ui_scale, &config, list[i]); 

        bigFont = ImGui::GetIO().Fonts->AddFontFromFileTTF(resources::getResourcePath("fonts/Roboto-Medium.ttf").c_str(), 45.0f * ui_scale);   //, &config, ranges);
        //hugeFont = ImGui::GetIO().Fonts->AddFontFromFileTTF(resources::getResourcePath("fonts/Roboto-Medium.ttf").c_str(), 128.0f * ui_scale); //, &config, ranges);
    }
}