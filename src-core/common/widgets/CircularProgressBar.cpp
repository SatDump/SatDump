#include "CircularProgressBar.h"

void CircularProgressBar(const char *label, float progress, ImVec2 size, ImVec4 color)
{
    ImGuiWindow *window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return;

    ImGui::BeginGroup();
    ImGui::PushID(label);

    ImVec2 pos = ImGui::GetCursorScreenPos();
    float circleRadius = ImMin(size.x, size.y) * 0.5f;
    ImVec2 center = ImVec2(pos.x + circleRadius, pos.y + circleRadius);

    float arcAngle = 2.0f * IM_PI * progress;

    const ImU32 bgColor = ImGui::GetColorU32(ImVec4(0.2f, 0.2f, 0.2f, 0.8f));
    window->DrawList->AddCircleFilled(center, circleRadius + 5, bgColor, 32);

    if (progress > 0.0f)
    {
        ImVec2 innerPos(center.x - circleRadius, center.y - circleRadius);
        ImVec2 outerPos(center.x + circleRadius, center.y + circleRadius);
        window->DrawList->PathArcTo(center, circleRadius, -IM_PI * 0.5f, -IM_PI * 0.5f + arcAngle, 32);
        window->DrawList->PathStroke(ImGui::GetColorU32(color), false, 10.0f);
    }

    char textBuffer[32];
    snprintf(textBuffer, sizeof(textBuffer), "%.0f%%", progress * 100.0f);
    ImVec2 textSize = ImGui::CalcTextSize(textBuffer);
    ImVec2 textPos = ImVec2(center.x - textSize.x * 0.5f, center.y - textSize.y * 0.5f);
    window->DrawList->AddText(textPos, ImGui::GetColorU32(ImGuiCol_Text), textBuffer);

    ImGui::PopID();
    ImGui::EndGroup();
}
