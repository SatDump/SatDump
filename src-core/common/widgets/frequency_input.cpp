#include "imgui/imgui.h"
#include "core/style.h"
#include "frequency_input.h"
#include "imgui/imgui_internal.h"
#include "core/module.h"

namespace widgets
{
	bool FrequencyInput(const char *label, double *frequency_mhz)
	{
		//Set up
		ImGui::BeginGroup();
		ImGui::PushID(label);
		double change_by = 0;
		bool num_started = false;
		std::string this_id;
		ImVec2 screen_pos;
		ImVec2 pos = ImGui::GetCursorPos();
		pos.x += 3 * ui_scale;
		ImGui::PushFont(style::freqFont);
		ImVec2 dot_size = ImGui::CalcTextSize(".");
		ImVec2 digit_size = ImGui::CalcTextSize("0");
		int mouse_wheel = ImGui::GetIO().MouseWheel;
		uint64_t int_freq = *frequency_mhz * 10e5;
		float offset = 0.0f;

		for (int i = 11; i >= 0; i--)
		{
			//Render the digit
			ImGui::SetCursorPos(pos);
			int this_place = (int_freq / (uint64_t)pow(10, i) % 10);
			num_started = num_started || this_place != 0;
			if (this_place == 0 && !num_started)
				ImGui::TextDisabled("%d", this_place);
			else
				ImGui::Text("%d", this_place);

			//Handle "up" events
			this_id = "topbutton" + std::to_string(i);
			ImGui::SetCursorPos(pos);
			screen_pos = ImGui::GetCursorScreenPos();
			if(ImGui::InvisibleButton(this_id.c_str(), ImVec2(digit_size.x, digit_size.y / 2), ImGuiButtonFlags_Repeat))
				change_by += pow(10, i - 6);
			ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelY, ImGuiInputFlags_CondHovered);
			if (ImGui::IsItemHovered())
			{
				//Handle mouse wheel (can be up or down)
				change_by += mouse_wheel * pow(10, i - 6);

				//Draw rect
				ImDrawList* draw_list = ImGui::GetWindowDrawList();
				ImGuiStyle style = ImGui::GetStyle();
				draw_list->AddRectFilled(screen_pos, ImVec2(screen_pos.x + digit_size.x, screen_pos.y + (digit_size.y / 2)),
					ImGui::ColorConvertFloat4ToU32(ImVec4(0.75, 0.75, 0.75, 0.5)), style.ChildRounding);
			}

			//Handle "down" events
			this_id = "bottombutton" + std::to_string(i);
			ImGui::SetCursorPos(ImVec2(pos.x, pos.y + (digit_size.y / 2)));
			screen_pos = ImGui::GetCursorScreenPos();
			if (ImGui::InvisibleButton(this_id.c_str(), ImVec2(digit_size.x, digit_size.y / 2), ImGuiButtonFlags_Repeat))
				change_by -= pow(10, i - 6);
			ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelY, ImGuiInputFlags_CondHovered);
			if (ImGui::IsItemHovered())
			{
				//Handle mouse wheel (can be up or down)
				change_by += mouse_wheel * pow(10, i - 6);

				//Draw rect
				ImDrawList* draw_list = ImGui::GetWindowDrawList();
				ImGuiStyle style = ImGui::GetStyle();
				draw_list->AddRectFilled(screen_pos, ImVec2(screen_pos.x + digit_size.x, screen_pos.y + (digit_size.y / 2)),
					ImGui::ColorConvertFloat4ToU32(ImVec4(0.75, 0.75, 0.75, 0.5)), style.ChildRounding);
			}

			//Add decimal, if needed
			pos.x += digit_size.x;
			if (i % 3 == 0 && i != 0)
			{
				ImGui::SetCursorPos(pos);
				if(num_started)
					ImGui::Text("%s", ".");
				else
					ImGui::TextDisabled("%s", ".");
				pos.x += dot_size.x;
			}
		}

		//Display Label
		ImGui::PopFont();
		ImGui::SetCursorPos(ImVec2(pos.x + (5 * ui_scale), pos.y + (digit_size.y - ImGui::CalcTextSize(label).y)));
		ImGui::TextDisabled("%s", label);
		ImGui::PopID();
		ImGui::EndGroup();

		// Finish up
		if (*frequency_mhz + change_by <= 0 || *frequency_mhz + change_by > 10e5)
			change_by = 0;
		
		*frequency_mhz += change_by;
		return change_by != 0;
	}
}