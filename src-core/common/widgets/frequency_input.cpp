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
		ImGuiContext &g = *GImGui;
		ImGuiStyle style = ImGui::GetStyle();
		ImVec2 pos = ImGui::GetCursorPos();
		static ImGuiID enable_temp_input_on = 0;
		static bool first_show_temp = false;
		ImGui::PushID(label);
		const ImGuiID id = ImGui::GetID(label);
		std::string display_label(label);
		size_t tag_start = display_label.find_first_of('#');
		if (tag_start != std::string::npos)
			display_label.erase(tag_start);

		if (g.NavActivateId == id && (g.NavActivateFlags & ImGuiActivateFlags_PreferInput))
		{
			enable_temp_input_on = id;
			first_show_temp = true;
		}

		if (enable_temp_input_on != id)
		{
			const float w = ImGui::CalcItemWidth();
			const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);
			const ImVec2 frame_size = ImVec2(w, label_size.y + style.FramePadding.y * 2.0f);
			const ImVec2 total_size = ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f);
			const ImRect frame_bb(pos, ImVec2(pos.x + frame_size.x, pos.y + frame_size.y));
			const ImRect total_bb(frame_bb.Min, ImVec2(frame_bb.Max.x + total_size.x, frame_bb.Max.y + total_size.y));
			ImGui::ItemAdd(total_bb, id, &frame_bb, ImGuiItemFlags_Inputable);
		}

		// Old-style input field
		else
		{
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2 * ui_scale);
			double frequency_mhz = *frequency_hz / 1e6;
			if (first_show_temp)
				ImGui::SetKeyboardFocusHere();
			bool retval = ImGui::InputDouble("##tempinput", &frequency_mhz);
			if (!first_show_temp && !ImGui::IsItemActive())
				enable_temp_input_on = 0;
			ImGui::SameLine();
			if (ImGui::Button(("M" + display_label).c_str()))
				enable_temp_input_on = 0;

			first_show_temp = false;
			if (retval)
				*frequency_hz = frequency_mhz * 1e6;
			ImGui::PopID();
			return retval;
		}

		// Modern input
		ImGuiIO& io = ImGui::GetIO();
		int64_t change_by = 0;
		bool num_started = false;
		std::string this_id;
		ImVec2 screen_pos;
		pos.x += 1 * ui_scale;
		ImGui::PushFont(style::freqFont);
		ImVec2 dot_size = ImGui::CalcTextSize(".");
		ImVec2 digit_size = ImGui::CalcTextSize("0");
		float offset = 0.0f;

		for (int i = 11; i >= 0; i--)
		{
			// Render the digit
			ImGui::SetCursorPos(pos);
			int this_place = (*frequency_hz / (uint64_t)pow(10, i) % 10);
			num_started = num_started || this_place != 0;
			if (!num_started)
				ImGui::TextDisabled("%d", this_place);
			else
				ImGui::Text("%d", this_place);

			// Handle "up" events
			this_id = "topbutton" + std::to_string(i);
			ImGui::SetCursorPos(pos);
			screen_pos = ImGui::GetCursorScreenPos();
			if (ImGui::InvisibleButton(this_id.c_str(), ImVec2(digit_size.x, digit_size.y / 2), ImGuiButtonFlags_Repeat | ImGuiButtonFlags_NoNavFocus))
			{
				if (io.KeyCtrl)
				{
					enable_temp_input_on = id;
					first_show_temp = true;
				}
				else
					change_by += pow(10, i);
			}
			ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelY, ImGuiInputFlags_CondHovered);
			if (ImGui::IsItemHovered())
			{
				//Handle mouse wheel (can be up or down)
				change_by += io.MouseWheel * pow(10, i);

				//Draw rect
				ImDrawList* draw_list = ImGui::GetWindowDrawList();
				draw_list->AddRectFilled(screen_pos, ImVec2(screen_pos.x + digit_size.x, screen_pos.y + (digit_size.y / 2)),
					ImGui::ColorConvertFloat4ToU32(ImVec4(0.596, 0.728, 0.884, 0.5)), style.ChildRounding);
			}

			//Handle "down" events
			this_id = "bottombutton" + std::to_string(i);
			ImGui::SetCursorPos(ImVec2(pos.x, pos.y + (digit_size.y / 2)));
			screen_pos = ImGui::GetCursorScreenPos();
			if (ImGui::InvisibleButton(this_id.c_str(), ImVec2(digit_size.x, digit_size.y / 2), ImGuiButtonFlags_Repeat | ImGuiButtonFlags_NoNavFocus))
			{
				if (io.KeyCtrl)
				{
					enable_temp_input_on = id;
					first_show_temp = true;
				}
				else
					change_by -= pow(10, i);
			}
			ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelY, ImGuiInputFlags_CondHovered);
			if (ImGui::IsItemHovered())
			{
				//Handle mouse wheel (can be up or down)
				change_by += io.MouseWheel * pow(10, i);

				//Draw rect
				ImDrawList* draw_list = ImGui::GetWindowDrawList();
				ImGuiStyle style = ImGui::GetStyle();
				draw_list->AddRectFilled(screen_pos, ImVec2(screen_pos.x + digit_size.x, screen_pos.y + (digit_size.y / 2)),
					ImGui::ColorConvertFloat4ToU32(ImVec4(0.596, 0.728, 0.884, 0.5)), style.ChildRounding);
			}

			//Add decimal, if needed
			pos.x += digit_size.x;
			if (i % 3 == 0 && i != 0)
			{
				ImGui::SetCursorPos(pos);
				if (num_started)
					ImGui::Text("%s", ".");
				else
					ImGui::TextDisabled("%s", ".");
				pos.x += dot_size.x;
			}
		}

		// Display Label
		ImGui::PopFont();
		ImVec2 label_size = ImGui::CalcTextSize(label);
		float button_height = label_size.y + style.FramePadding.y * 2.0f;
		ImGui::SetCursorPos(ImVec2(pos.x + (5 * ui_scale), pos.y + 1 * ui_scale + (digit_size.y / 2 - button_height / 2)));
		if(ImGui::Button(display_label.c_str()))
		{
			enable_temp_input_on = id;
			first_show_temp = true;
		}

		// Finish up
		ImGui::PopID();
		if (*frequency_hz + change_by <= 0 || *frequency_hz + change_by > 1e12)
			change_by = 0;

		*frequency_hz += change_by;
		return change_by != 0;
	}
}