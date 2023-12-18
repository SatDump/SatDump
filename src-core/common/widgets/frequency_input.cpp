#include "imgui/imgui.h"
#include "core/style.h"
#include "frequency_input.h"
#include "imgui/imgui_internal.h"
#include "core/module.h"

namespace widgets
{
	bool FrequencyInput(const char *label, uint64_t *frequency_hz)
	{
		//Set up
		ImGui::BeginGroup();
		ImGui::PushID(label);
		int64_t change_by = 0;
		bool num_started = false;
		std::string this_id;
		ImVec2 screen_pos;
		ImVec2 pos = ImGui::GetCursorPos();
		pos.x += 2 * ui_scale;
		ImGui::PushFont(style::freqFont);
		ImVec2 dot_size = ImGui::CalcTextSize(".");
		ImVec2 digit_size = ImGui::CalcTextSize("0");
		int mouse_wheel = ImGui::GetIO().MouseWheel;
		float offset = 0.0f;

		for (int i = 11; i >= 0; i--)
		{
			//Render the digit
			ImGui::SetCursorPos(pos);
			int this_place = (*frequency_hz / (uint64_t)pow(10, i) % 10);
			num_started = num_started || this_place != 0;
			if (!num_started)
				ImGui::TextDisabled("%d", this_place);
			else
				ImGui::Text("%d", this_place);

			//Handle "up" events
			this_id = "topbutton" + std::to_string(i);
			ImGui::SetCursorPos(pos);
			screen_pos = ImGui::GetCursorScreenPos();
			if(ImGui::InvisibleButton(this_id.c_str(), ImVec2(digit_size.x, digit_size.y / 2), ImGuiButtonFlags_Repeat))
				change_by += pow(10, i);
			ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelY, ImGuiInputFlags_CondHovered);
			if (ImGui::IsItemHovered())
			{
				//Handle mouse wheel (can be up or down)
				change_by += mouse_wheel * pow(10, i);

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
				change_by -= pow(10, i);
			ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelY, ImGuiInputFlags_CondHovered);
			if (ImGui::IsItemHovered())
			{
				//Handle mouse wheel (can be up or down)
				change_by += mouse_wheel * pow(10, i);

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

		// Remove hidden tag and display
		std::string display_label(label);
		size_t tag_start = display_label.find_first_of('#');
		if(tag_start != std::string::npos)
			display_label.erase(tag_start);
		ImGui::PopFont();
		ImGui::SetCursorPos(ImVec2(pos.x + (5 * ui_scale), pos.y + (digit_size.y - ImGui::CalcTextSize(label).y - 2 * ui_scale)));
		ImGui::TextUnformatted(display_label.c_str());
		ImGui::PopID();
		ImGui::EndGroup();

		// Finish up
		if (*frequency_hz + change_by <= 0 || *frequency_hz + change_by > 1e12)
			change_by = 0;
		
		*frequency_hz += change_by;
		return change_by != 0;
	}
}