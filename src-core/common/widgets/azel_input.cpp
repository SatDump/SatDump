#include "imgui/imgui.h"
#include "core/style.h"
#include "core/backend.h"
#include "frequency_input.h"
#include "imgui/imgui_internal.h"
#include <cmath>

namespace widgets
{
	inline void helper_left(int &i, ImVec2 &digit_size, ImVec2 &screen_pos, float &dot_width, bool &top)
	{
		if (i == 5)
			return;
		float x = screen_pos.x - digit_size.x / 2;
		if ((i + 1) % 3 == 0)
			x -= dot_width;
		backend::setMousePos(x, screen_pos.y + (top ? digit_size.y * 0.75 : digit_size.y / 4));
	}

	inline void helper_right(int &i, ImVec2 &digit_size, ImVec2 &screen_pos, float &dot_width, bool &top)
	{
		if (i == 0)
			return;
		float x = screen_pos.x + digit_size.x * 1.5;
		if (i % 3 == 0)
			x += dot_width;
		backend::setMousePos(x, screen_pos.y + (top ? digit_size.y * 0.75 : digit_size.y / 4));
	}

	inline void digit_helper(int &i, int64_t *wip_val, int64_t &change_by,
							 ImVec2 &digit_size, ImVec2 &screen_pos, float &dot_width, float &rounding, bool top)
	{
		if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
			change_by = -(*wip_val % (int64_t)pow(10, i + 1));
		if (ImGui::IsItemHovered())
		{
			// Handle Enter
			if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter))
				change_by = -(*wip_val % (int64_t)pow(10, i + 1));

			// Handle Arrow Keys
			if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
				change_by += pow(10, i);
			if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
				change_by -= pow(10, i);
			if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow) || ImGui::IsKeyPressed(ImGuiKey_Backspace))
				helper_left(i, digit_size, screen_pos, dot_width, top);
			if (ImGui::IsKeyPressed(ImGuiKey_RightArrow))
				helper_right(i, digit_size, screen_pos, dot_width, top);

			// Handle number keys
			ImGuiIO &io = ImGui::GetIO();
			for (auto &this_char : io.InputQueueCharacters)
			{
				if (this_char >= '0' && this_char <= '9')
				{
					change_by = pow(10, i) * (this_char - '0' - (int64_t)(*wip_val / (int64_t)pow(10, i) % 10));
					helper_right(i, digit_size, screen_pos, dot_width, top);
				}
			}

			// Draw rect
			ImDrawList *draw_list = ImGui::GetWindowDrawList();
			draw_list->AddRectFilled(screen_pos, ImVec2(screen_pos.x + digit_size.x, screen_pos.y + (digit_size.y / 2)),
									 style::theme.freq_highlight, rounding);
		}
	}

	bool AzElInput(const char *label, float *az_el_val, bool allow_edit)
	{
		int64_t wip_val = std::round((*az_el_val) * 1e4) / 10;

		// Set up
		ImGuiContext &g = *GImGui;
		ImGuiStyle style = ImGui::GetStyle();
		ImDrawList *draw_list = ImGui::GetWindowDrawList();
		ImVec2 pos = ImGui::GetCursorPos();
		static ImGuiID enable_temp_input_on = 0;
		static bool first_show_temp = false;
		ImGui::PushID(label);
		const ImGuiID id = ImGui::GetID(label);
		const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);
		const ImVec2 frame_size = ImVec2(ImGui::CalcItemWidth(), label_size.y + style.FramePadding.y * 2.0f);
		const ImVec2 button_size = ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, frame_size.y);
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
			const ImRect frame_bb(pos, ImVec2(pos.x + frame_size.x, pos.y + frame_size.y));
			const ImRect total_bb(frame_bb.Min, ImVec2(frame_bb.Max.x + button_size.x, frame_bb.Max.y + button_size.y));
			ImGui::ItemAdd(total_bb, id, &frame_bb, ImGuiItemFlags_Inputable);
		}

		// Old-style input field
		else if(allow_edit)
		{
			if (first_show_temp)
				ImGui::SetKeyboardFocusHere();
			bool retval = ImGui::InputFloat("##tempinput", az_el_val);
			if (!first_show_temp && !ImGui::IsItemActive())
				enable_temp_input_on = 0;
			ImGui::SameLine();

			first_show_temp = false;
			ImGui::PopID();
			return retval;
		}

		// Modern input
		int64_t change_by = 0;
		bool num_started = false;
		std::string this_id;
		ImVec2 screen_pos;
		float freq_size = style::bigFont->FontSize;
		pos.x += 1 * ui_scale;
		ImVec2 dot_size = style::bigFont->CalcTextSizeA(freq_size, FLT_MAX, 0.0f, ".");
		ImVec2 digit_size = style::bigFont->CalcTextSizeA(freq_size, FLT_MAX, 0.0f, "0");

		// Calculate the total size
		float target_size = digit_size.x * 12 + dot_size.x * 3;
		float available_size = ImGui::GetContentRegionAvail().x - button_size.x - style.ItemSpacing.x * 2;
		if (/*scale != 0.0f ||*/ available_size < target_size)
		{
			float freq_scale;
			// if (scale == 0.0f)
			freq_scale = (available_size < 150.0f * ui_scale ? 150.0f * ui_scale : available_size) / target_size;
			//	else
			//		freq_scale = scale;
			freq_size *= freq_scale;
			dot_size.x *= freq_scale;
			dot_size.y *= freq_scale;
			digit_size.x *= freq_scale;
			digit_size.y *= freq_scale;
		}

		for (int i = 6; i >= 0; i--)
		{
			// Render the digit
			ImGui::SetCursorPos(pos);
			screen_pos = ImGui::GetCursorScreenPos();
			char place_char[2];
			int this_place = (wip_val < 0) ? (-wip_val / (uint64_t)pow(10, i) % 10) : (wip_val / (uint64_t)pow(10, i) % 10);

			if (i == 6)
			{
				if (wip_val < 0)
				{
					place_char[0] = '-';
					place_char[1] = 0;
				}
				else
				{
					place_char[0] = '+';
					place_char[1] = 0;
				}
			}
			else
			{
				sprintf(place_char, "%d", this_place);
			}

			num_started = num_started || this_place != 0;

			ImU32 font_color;
			if (!num_started)
				font_color = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_TextDisabled]);
			else
				font_color = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_Text]);

			draw_list->AddText(style::bigFont, freq_size, screen_pos, font_color, place_char);

			if (i != 6 && allow_edit)
			{
				// Handle "up" events
				this_id = "topbutton" + std::to_string(i);
				ImGui::SetCursorPos(pos);
				if (ImGui::InvisibleButton(this_id.c_str(), ImVec2(digit_size.x, digit_size.y / 2), ImGuiButtonFlags_Repeat | ImGuiButtonFlags_NoNavFocus))
				{
					if (ImGui::GetIO().KeyCtrl)
					{
						enable_temp_input_on = id;
						first_show_temp = true;
					}
					else
						change_by += pow(10, i);
				}
				digit_helper(i, &wip_val, change_by, digit_size, screen_pos, dot_size.x, style.ChildRounding, true);

				// Handle "down" events
				this_id = "bottombutton" + std::to_string(i);
				ImGui::SetCursorPos(ImVec2(pos.x, pos.y + (digit_size.y / 2)));
				screen_pos = ImGui::GetCursorScreenPos();
				if (ImGui::InvisibleButton(this_id.c_str(), ImVec2(digit_size.x, digit_size.y / 2), ImGuiButtonFlags_Repeat | ImGuiButtonFlags_NoNavFocus))
				{
					if (ImGui::GetIO().KeyCtrl)
					{
						enable_temp_input_on = id;
						first_show_temp = true;
					}
					else
						change_by -= pow(10, i);
				}
				digit_helper(i, &wip_val, change_by, digit_size, screen_pos, dot_size.x, style.ChildRounding, false);
			}

			// Add decimal, if needed
			pos.x += digit_size.x;
			if (i % 3 == 0 && i != 0 && i != 6)
			{
				ImGui::SetCursorPos(pos);
				screen_pos = ImGui::GetCursorScreenPos();
				draw_list->AddText(style::bigFont, freq_size, screen_pos, font_color, ".");
				pos.x += dot_size.x;
			}
		}

		// Finish up
		ImGui::SetCursorPosY(pos.y + digit_size.y + style.ItemSpacing.y);
		ImGui::PopID();

		wip_val += change_by;
		if (std::abs(wip_val) >= 1e6)
			change_by = 0;

		if (change_by != 0)
		{
			*az_el_val = wip_val / 1e3;
			return true;
		}
		else
			return false;
	}
}
