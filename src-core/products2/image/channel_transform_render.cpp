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
        if (ImGui::RadioButton("Affine InterpX", d_type == TYPE_AFFINE_INTERPX))
            d_type = TYPE_AFFINE_INTERPX;
        if (ImGui::RadioButton("InterpXY", d_type == TYPE_INTERP_XY))
            d_type = TYPE_INTERP_XY;

        ImGui::Spacing();

        if (d_type == TYPE_AFFINE ||
            d_type == TYPE_AFFINE_SLANTX ||
            d_type == TYPE_AFFINE_INTERPX)
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

        if (d_type == TYPE_AFFINE_INTERPX)
        {
            ImGui::Separator();
            int i = 0;
            for (auto &v : interpx_points)
            {
                std::string id = "##none" + std::to_string(i++);
                ImGui::SetNextItemWidth(100);
                ImGui::InputDouble((" =>" + id + "x1").c_str(), &v.first); // TODOREWORK do like interp_xy
                ImGui::SameLine();
                ImGui::SetNextItemWidth(100);
                ImGui::InputDouble((id + "x2").c_str(), &v.second);
                ImGui::Separator();
            }

            if (ImGui::Button("Add"))
                interpx_points.push_back({0, 0});
            if (ImGui::Button("Solve"))
            {
                nlohmann::json v = *this; // TODOREWORK remove
                *this = v;
            }
        }

        if (d_type == TYPE_INTERP_XY)
        {
            ImGui::Separator();
            int i = 0;
            for (auto &v : interp_xy_points)
            {
                std::string id = "##none" + std::to_string(i++);
                ImGui::SetNextItemWidth(100);
                ImGui::InputDouble((id + "x1").c_str(), &v.first.first);
                ImGui::SameLine();
                ImGui::SetNextItemWidth(100);
                ImGui::InputDouble((id + "y1").c_str(), &v.first.second);
                ImGui::SetNextItemWidth(100);
                ImGui::InputDouble((id + "x2").c_str(), &v.second.first);
                ImGui::SameLine();
                ImGui::SetNextItemWidth(100);
                ImGui::InputDouble((id + "y2").c_str(), &v.second.second);
                ImGui::Separator();
            }

            if (ImGui::Button("Add"))
                interp_xy_points.push_back({{0, 0}, {0, 0}});
            if (ImGui::Button("Solve"))
            {
                nlohmann::json v = *this; // TODOREWORK remove
                *this = v;
            }
        }

        ImGui::EndGroup();
        ImGui::PopID();
    }
}