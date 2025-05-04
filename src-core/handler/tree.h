#pragma once

#include "core/style.h"
#include "imgui/imgui_internal.h"
#include "logger.h"

struct TreeDrawerToClean
{
    float SmallOffsetX = 11.0f - 22; // for now, a hardcoded value; should take into account tree indent size

    ImDrawList *drawList = nullptr;

    ImVec2 verticalLineStart = {0, 0};

    ImVec2 verticalLineEnd = {0, 0};

    void start()
    {
        drawList = ImGui::GetWindowDrawList();

        verticalLineStart = ImGui::GetCursorScreenPos();
        verticalLineStart.x += SmallOffsetX * ui_scale; // to nicely line up with the arrow symbol
        verticalLineEnd = verticalLineStart;
    }

    bool node(std::string icon)
    {
        const float HorizontalTreeLineSize = 8.0f * ui_scale;                              // chosen arbitrarily
        const ImRect childRect = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax()); // RenderTree(child);
        const float midpoint = (childRect.Min.y + childRect.Max.y) / 2.0f;
        drawList->AddLine(ImVec2(verticalLineStart.x, midpoint), ImVec2(verticalLineStart.x + HorizontalTreeLineSize, midpoint), style::theme.treeview_icon);
        drawList->AddText(ImVec2(verticalLineStart.x + HorizontalTreeLineSize * 2.0f, childRect.Min.y), style::theme.treeview_icon, icon.c_str());
        verticalLineEnd.y = midpoint;
        return false;
    }

    float end()
    {
        drawList->AddLine(verticalLineStart, verticalLineEnd, style::theme.treeview_icon);

        return verticalLineEnd.y - verticalLineStart.y;
    }
};
