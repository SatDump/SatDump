#include "switch.h"

void ToggleButton(const char* str_id, int* v)
{
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    float height = ImGui::GetFrameHeight() * 0.75f;
    float width = height * 2.0f;

    ImGui::InvisibleButton(str_id, ImVec2(width, height));
    if (ImGui::IsItemClicked())
        *v = !*v;

    float t = *v ? 1.0f : 0.0f;

    ImGuiContext& g = *GImGui;
    float ANIM_SPEED = 0.04f;
    if (g.LastActiveId == g.CurrentWindow->GetID(str_id))// && g.LastActiveIdTimer < ANIM_SPEED)
    {
        float t_anim = ImSaturate(g.LastActiveIdTimer / ANIM_SPEED);
        t = *v ? (t_anim) : (1.0f - t_anim);
    }
    int off = 2;

    draw_list->AddRectFilled(p, ImVec2(p.x + width, p.y + height), IM_COL32(35, 37, 38, 255), 2);
    draw_list->AddRectFilled(ImVec2(p.x + t * height + off, p.y + off), ImVec2(p.x + (t + 1) * height - off, p.y + height - off),  IM_COL32(61, 133, 224, 255), 2);
}