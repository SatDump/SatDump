#include "channel_transform.h"
#include "imgui/imgui.h"

namespace satdump
{
    void ChannelTransform::render()
    {
        ImGui::PushID(this);
        ImGui::BeginGroup();

        ImGui::Text("Type :");
        if (ImGui::RadioButton("Invalid", d_type == TYPE_INVALID))
            d_type = TYPE_INVALID;
        if (ImGui::RadioButton("None", d_type == TYPE_NONE))
            d_type = TYPE_NONE;
        if (ImGui::RadioButton("Affine", d_type == TYPE_AFFINE))
            d_type = TYPE_AFFINE;
        if (ImGui::RadioButton("Affine SlantX", d_type == TYPE_AFFINE_SLANTX))
            d_type = TYPE_AFFINE_SLANTX;

        ImGui::Spacing();

        if (d_type == TYPE_AFFINE || d_type == TYPE_AFFINE_SLANTX)
        {
            ImGui::InputDouble("affine_a_x", &affine_a_x);
            ImGui::InputDouble("affine_a_y", &affine_a_y);
            ImGui::InputDouble("affine_b_x", &affine_b_x);
            ImGui::InputDouble("affine_b_y", &affine_b_y);
        }

        if (d_type == TYPE_AFFINE_SLANTX)
        {
            ImGui::InputDouble("slantx_center", &slantx_center);
            ImGui::InputDouble("slantx_ampli", &slantx_ampli);
        }

        ImGui::EndGroup();
        ImGui::PopID();
    }
}