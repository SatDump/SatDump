#include <algorithm>
#include "stepped_slider.h"
#include "imgui/imgui_internal.h"

namespace widgets
{
    bool SteppedSliderInt(const char* label, int* v, int v_min, int v_max, int v_rate, const char* format, ImGuiSliderFlags flags)
    {
        ImGuiStyle style = ImGui::GetStyle();
        const float button_size = ImGui::GetFrameHeight();
        const float slider_width = std::max(1.0f, ImGui::CalcItemWidth() - (button_size + style.ItemInnerSpacing.x) * 2);

        ImGui::BeginGroup();
        ImGui::PushID(label);
        ImGui::SetNextItemWidth(slider_width);
        bool retval = ImGui::SliderInt("##slider", v, v_min, v_max, format, flags);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(style.FramePadding.y, style.FramePadding.y));
        ImGui::SameLine(0, style.ItemInnerSpacing.x);
        ImGui::PushButtonRepeat(true);
        if (ImGui::Button("-", ImVec2(button_size, button_size)))
        {
            GImGui->ActiveIdPreviousFrameHasBeenEditedBefore = true;
            retval = true;
            if (*v - v_rate < v_min)
                *v = v_min;
            else
                *v -= v_rate;
        }
        ImGui::SameLine(0, style.ItemInnerSpacing.x);
        if (ImGui::Button("+", ImVec2(button_size, button_size)))
        {
            GImGui->ActiveIdPreviousFrameHasBeenEditedBefore = true;
            retval = true;
            if (*v + v_rate > v_max)
                *v = v_max;
            else
                *v += v_rate;
        }
        ImGui::PopButtonRepeat();
        ImGui::SameLine(0, style.ItemInnerSpacing.x);
        ImGui::TextUnformatted(label);
        ImGui::PopStyleVar();
        ImGui::PopID();
        ImGui::EndGroup();

        return retval;
    }
    
    bool SteppedSliderFloat(const char* label, float* v, float v_min, float v_max, float v_rate, const char* format, ImGuiSliderFlags flags)
    {
        ImGuiStyle style = ImGui::GetStyle();
        const float button_size = ImGui::GetFrameHeight();
        const float slider_width = std::max(1.0f, ImGui::CalcItemWidth() - (button_size + style.ItemInnerSpacing.x) * 2);

        ImGui::BeginGroup();
        ImGui::PushID(label);
        ImGui::SetNextItemWidth(slider_width);
        bool retval = ImGui::SliderFloat("##slider", v, v_min, v_max, format, flags);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(style.FramePadding.y, style.FramePadding.y));
        ImGui::SameLine(0, style.ItemInnerSpacing.x);
        ImGui::PushButtonRepeat(true);
        if (ImGui::Button("-", ImVec2(button_size, button_size)))
        {
            GImGui->ActiveIdPreviousFrameHasBeenEditedBefore = true;
            retval = true;
            if (ImGui::IsKeyDown(ImGuiKey_ModShift))
                v_rate /= 10;
            else if (ImGui::IsKeyDown(ImGuiKey_ModCtrl))
                v_rate /= 100;

            if (*v - v_rate < v_min)
                *v = v_min;
            else
                *v -= v_rate;
        }
        ImGui::SameLine(0, style.ItemInnerSpacing.x);
        if (ImGui::Button("+", ImVec2(button_size, button_size)))
        {
            GImGui->ActiveIdPreviousFrameHasBeenEditedBefore = true;
            retval = true;
            if (ImGui::IsKeyDown(ImGuiKey_ModShift))
                v_rate /= 10;
            else if (ImGui::IsKeyDown(ImGuiKey_ModCtrl))
                v_rate /= 100;

            if (*v + v_rate > v_max)
                *v = v_max;
            else
                *v += v_rate;
        }
        ImGui::PopButtonRepeat();
        ImGui::SameLine(0, style.ItemInnerSpacing.x);
        ImGui::TextUnformatted(label);
        ImGui::PopStyleVar();
        ImGui::PopID();
        ImGui::EndGroup();

        return retval;
    }
}