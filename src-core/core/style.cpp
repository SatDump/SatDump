#define SATDUMP_DLL_EXPORT 1

#include <filesystem>
#include "style.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "nlohmann/json_utils.h"
#include "logger.h"
#include "config.h"
#include "module.h"
#include "backend.h"
#include "resources.h"

#ifdef __APPLE__
#include <CoreGraphics/CGDirectDisplay.h>
#endif

namespace style
{
    SATDUMP_DLL Theme theme;
    SATDUMP_DLL ImFont *baseFont;
    SATDUMP_DLL ImFont *bigFont;
    //SATDUMP_DLL ImFont *hugeFont;

    void hexToImVec4(std::string color_hex, ImVec4 *this_color)
    {
        color_hex.erase(std::remove_if(color_hex.begin(), color_hex.end(),
            [&](const char c) { return !std::isxdigit(c); }), color_hex.end());
        if (color_hex.size() != 8)
        {
            logger->debug("Invalid color code %s", color_hex.c_str());
            return;
        }

        this_color->x = (float)std::stoi(color_hex.substr(0, 2), 0, 16) / 255.0f;
        this_color->y = (float)std::stoi(color_hex.substr(2, 2), 0, 16) / 255.0f;
        this_color->z = (float)std::stoi(color_hex.substr(4, 2), 0, 16) / 255.0f;
        this_color->w = (float)std::stoi(color_hex.substr(6, 2), 0, 16) / 255.0f;
    }

    void setStyle()
    {
        // Set standard theme info
        ui_scale = backend::device_scale * satdump::config::main_cfg["user_interface"]["manual_dpi_scaling"]["value"].get<float>();
        ImGuiStyle &style = ImGui::GetStyle();
        style = ImGuiStyle();
        theme = Theme();

        // Load Theme File
        nlohmann::json data;
        try
        {
            std::string selected_theme = satdump::config::main_cfg["user_interface"]["theme"]["value"];
            std::ifstream file(resources::getResourcePath("themes/" + selected_theme + ".json"));
            file >> data;
            file.close();
        }
        catch (std::exception&)
        {
            logger->error("Failed to load theme! There will be visual issues.");
            return;
        }

        // Set base theme
        bool light_mode = getValueOrDefault(data["light"], false);
        if (light_mode)
            ImGui::StyleColorsLight();
        else
            ImGui::StyleColorsDark();

        // ImGui sizes
        if (data.contains("ImGuiStyle") && data["ImGuiStyle"].is_object())
        {
            const std::map<std::string, float ImGuiStyle::*> style_map = {
                {"Alpha", &ImGuiStyle::Alpha},
                {"DisabledAlpha", &ImGuiStyle::DisabledAlpha},
                {"WindowRounding", &ImGuiStyle::WindowRounding},
                {"WindowBorderSize", &ImGuiStyle::WindowBorderSize},
                {"ChildRounding", &ImGuiStyle::ChildRounding},
                {"ChildBorderSize", &ImGuiStyle::ChildBorderSize},
                {"PopupRounding", &ImGuiStyle::PopupRounding},
                {"PopupBorderSize", &ImGuiStyle::PopupBorderSize},
                {"FrameRounding", &ImGuiStyle::FrameRounding},
                {"FrameBorderSize", &ImGuiStyle::FrameBorderSize},
                {"IndentSpacing", &ImGuiStyle::IndentSpacing},
                {"ScrollbarSize", &ImGuiStyle::ScrollbarSize},
                {"ScrollbarRounding", &ImGuiStyle::ScrollbarRounding},
                {"GrabMinSize", &ImGuiStyle::GrabMinSize},
                {"GrabRounding", &ImGuiStyle::GrabRounding},
                {"TabRounding", &ImGuiStyle::TabRounding},
                {"TabBorderSize", &ImGuiStyle::TabBorderSize},
                {"TabBarBorderSize", &ImGuiStyle::TabBarBorderSize}
            };

            for (auto& style_item : data["ImGuiStyle"].items())
            {
                if (!style_item.value().is_number_float())
                {
                    logger->debug("Invalid theme value for %s", style_item.key().c_str());
                    continue;
                }

                std::map<std::string, float ImGuiStyle::*>::const_iterator style_pos = style_map.find(style_item.key());
                if (style_pos != style_map.end())
                    style.*(style_pos->second) = style_item.value();
                else
                    logger->debug("Invalid theme attribute: %s", style_item.key().c_str());
            }
        }
        style.ScaleAllSizes(ui_scale);

        // Built-in ImGui colors
        if (data.contains("ImGuiColors") && data["ImGuiColors"].is_object())
        {
            const std::map<std::string, ImGuiCol> color_map = {
                {"Text", ImGuiCol_Text},
                {"TextDisabled", ImGuiCol_TextDisabled},
                {"WindowBg", ImGuiCol_WindowBg},
                {"ChildBg", ImGuiCol_ChildBg},
                {"PopupBg", ImGuiCol_PopupBg},
                {"Border", ImGuiCol_Border},
                {"BorderShadow", ImGuiCol_BorderShadow},
                {"FrameBg", ImGuiCol_FrameBg},
                {"FrameBgHovered", ImGuiCol_FrameBgHovered},
                {"FrameBgActive", ImGuiCol_FrameBgActive},
                {"TitleBg", ImGuiCol_TitleBg},
                {"TitleBgActive", ImGuiCol_TitleBgActive},
                {"TitleBgCollapsed", ImGuiCol_TitleBgCollapsed},
                {"MenuBarBg", ImGuiCol_MenuBarBg},
                {"ScrollbarBg", ImGuiCol_ScrollbarBg},
                {"ScrollbarGrab", ImGuiCol_ScrollbarGrab},
                {"ScrollbarGrabHovered", ImGuiCol_ScrollbarGrabHovered},
                {"ScrollbarGrabActive", ImGuiCol_ScrollbarGrabActive},
                {"CheckMark", ImGuiCol_CheckMark},
                {"SliderGrab", ImGuiCol_SliderGrab},
                {"SliderGrabActive", ImGuiCol_SliderGrabActive},
                {"Button", ImGuiCol_Button},
                {"ButtonHovered", ImGuiCol_ButtonHovered},
                {"ButtonActive", ImGuiCol_ButtonActive},
                {"Header", ImGuiCol_Header},
                {"HeaderHovered", ImGuiCol_HeaderHovered},
                {"HeaderActive", ImGuiCol_HeaderActive},
                {"Separator", ImGuiCol_Separator},
                {"SeparatorHovered", ImGuiCol_SeparatorHovered},
                {"SeparatorActive", ImGuiCol_SeparatorActive},
                {"ResizeGrip", ImGuiCol_ResizeGrip},
                {"ResizeGripHovered", ImGuiCol_ResizeGripHovered},
                {"ResizeGripActive", ImGuiCol_ResizeGripActive},
                {"Tab", ImGuiCol_Tab},
                {"TabHovered", ImGuiCol_TabHovered},
                {"TabActive", ImGuiCol_TabActive},
                {"TabUnfocused", ImGuiCol_TabUnfocused},
                {"TabUnfocusedActive", ImGuiCol_TabUnfocusedActive},
                {"PlotLines", ImGuiCol_PlotLines},
                {"PlotLinesHovered", ImGuiCol_PlotLinesHovered},
                {"PlotHistogram", ImGuiCol_PlotHistogram},
                {"PlotHistogramHovered", ImGuiCol_PlotHistogramHovered},
                {"TextSelectedBg", ImGuiCol_TextSelectedBg},
                {"DragDropTarget", ImGuiCol_DragDropTarget},
                {"NavHighlight", ImGuiCol_NavHighlight},
                {"NavWindowingHighlight", ImGuiCol_NavWindowingHighlight},
                {"NavWindowingDimBg", ImGuiCol_NavWindowingDimBg},
                {"ModalWindowDimBg", ImGuiCol_ModalWindowDimBg},
                {"TableHeaderBg", ImGuiCol_TableHeaderBg},
                {"TableBorderStrong", ImGuiCol_TableBorderStrong},
                {"TableBorderLight", ImGuiCol_TableBorderLight},
                {"TableRowBg", ImGuiCol_TableRowBg},
                {"TableRowBgAlt", ImGuiCol_TableRowBgAlt},
            };

            for (auto& color : data["ImGuiColors"].items())
            {
                if (!color.value().is_string())
                {
                    logger->debug("Invalid theme color for %s", color.key().c_str());
                    continue;
                }

                std::map<std::string, ImGuiCol>::const_iterator color_pos = color_map.find(color.key());
                if (color_pos != color_map.end())
                    hexToImVec4(color.value(), &style.Colors[color_pos->second]);
                else
                    logger->debug("Invalid theme color: %s", color.key().c_str());
            }
        }

        // Custom SatDump Colors
        if (data.contains("SatDumpColors") && data["SatDumpColors"].is_object())
        {
            const std::map<std::string, ImColor Theme::*> custom_color_map = {
                {"red", &Theme::red},
                {"green", &Theme::green},
                {"blue", &Theme::blue},
                {"yellow", &Theme::yellow},
                {"orange", &Theme::orange},
                {"cyan", &Theme::cyan},
                {"fuchsia", &Theme::fuchsia},
                {"magenta", &Theme::magenta},
                {"lavender", &Theme::lavender},
                {"light_green", &Theme::light_green},
                {"light_cyan", &Theme::light_cyan},
                {"constellation", &Theme::constellation},
                {"widget_bg", &Theme::widget_bg},
                {"frame_bg", &Theme::frame_bg},
                {"overlay_bg", &Theme::overlay_bg},
                {"freq_highlight", &Theme::freq_highlight}
            };

            for (auto& color : data["SatDumpColors"].items())
            {
                if (!color.value().is_string())
                {
                    logger->debug("Invalid theme color for %s", color.key().c_str());
                    continue;
                }

                std::map<std::string, ImColor Theme::*>::const_iterator color_pos = custom_color_map.find(color.key());
                if (color_pos != custom_color_map.end())
                    hexToImVec4(color.value(), &(theme.*(color_pos->second)).Value);
                else
                    logger->debug("Invalid theme color: %s", color.key().c_str());
            }
        }
    }

    void beginDisabled()
    {
        ImVec4 *colors = ImGui::GetStyle().Colors;
        ImVec4 frame_bg = colors[ImGuiCol_FrameBg];
        ImVec4 button = colors[ImGuiCol_Button];
        frame_bg.w *= 0.33f;
        button.w *= 0.33f;

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

    void setFonts(float dpi_scaling)
    {
        ImGuiIO &io = ImGui::GetIO();
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

        backend::rebuildFonts();
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
