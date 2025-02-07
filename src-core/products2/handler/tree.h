#pragma once

#include "imgui/imgui_internal.h"

struct TreeDrawerToClean
{

    const ImColor TreeLineColor = ImColor(128, 128, 128, 255); // ImGui::GetColorU32(ImGuiCol_Text);
    float SmallOffsetX = 11.0f - 22;                           // for now, a hardcoded value; should take into account tree indent size

    ImDrawList *drawList;

    ImVec2 verticalLineStart;

    ImVec2 verticalLineEnd;

    void start()
    {
        drawList = ImGui::GetWindowDrawList();

        verticalLineStart = ImGui::GetCursorScreenPos();
        verticalLineStart.x += SmallOffsetX; // to nicely line up with the arrow symbol
        verticalLineEnd = verticalLineStart;
    }

    bool node(std::string icon)
    {
        const float HorizontalTreeLineSize = 8.0f;                                         // chosen arbitrarily
        const ImRect childRect = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax()); // RenderTree(child);
        const float midpoint = (childRect.Min.y + childRect.Max.y) / 2.0f;
        drawList->AddLine(ImVec2(verticalLineStart.x, midpoint), ImVec2(verticalLineStart.x + HorizontalTreeLineSize, midpoint), TreeLineColor);
        drawList->AddText(ImVec2(verticalLineStart.x + HorizontalTreeLineSize * 2.0f, childRect.Min.y), TreeLineColor, icon.c_str());
        verticalLineEnd.y = midpoint;
        return false;
    }

    float end()
    {
        drawList->AddLine(verticalLineStart, verticalLineEnd, TreeLineColor);

        return verticalLineEnd.y - verticalLineStart.y;
    }
};
