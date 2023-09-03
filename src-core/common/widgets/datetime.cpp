#include "datetime.h"
#include "imgui/imgui.h"
#include "imgui/imgui_stdlib.h"
#include "core/module.h"

#ifdef _WIN32
#define timegm _mkgmtime
#endif

namespace widgets
{
	DateTimePicker::DateTimePicker(float input_time)
	{
		handle_input(input_time);
	}
	DateTimePicker::~DateTimePicker()
	{
	}
	void DateTimePicker::handle_input(float input_time)
	{
		time_t input_timet;
		if (input_time == -1)
		{
			auto_time = true;
			input_timet = time(NULL);
		}
		else
		{
			auto_time = false;
			input_timet = (time_t)input_time;
		}
		timestamp = gmtime(&input_timet);
		seconds_decimal = std::to_string(input_time - input_timet);
		size_t decimal_pos = seconds_decimal.find(".");
		seconds_decimal = seconds_decimal.substr(decimal_pos + 1);
		year_holder = timestamp->tm_year + 1900;
		month_holder = timestamp->tm_mon + 1;
	}
	float DateTimePicker::get()
	{
		if (auto_time)
			return -1.0f;
		else
			return stof(std::to_string(timegm(timestamp)) + "." + seconds_decimal);
	}
	void DateTimePicker::set(float input_time)
	{
		handle_input(input_time);
	}
	void DateTimePicker::draw()
	{
		std::string checkbox_label = "###dsauto";
		if (auto_time)
			checkbox_label = "Auto###dsauto";
		ImGui::Checkbox(checkbox_label.c_str(), &auto_time);

		if (!auto_time)
		{
			ImGui::SameLine();
			ImGuiStyle style = ImGui::GetStyle();
			ImU32 input_background = ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));

			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0, 0.0, 0.0, 0.0));
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, style.FramePadding.y));
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(3 * ui_scale, style.ItemSpacing.y));
			ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(3 * ui_scale, style.ItemInnerSpacing.y));

			//Calculate Draw Size
			ImVec2 slash_size = ImGui::CalcTextSize("/");
			ImVec2 dot_size = ImGui::CalcTextSize(".");
			ImVec2 digit_size = ImGui::CalcTextSize("0");
			int date_width = (slash_size.x * 2) + (digit_size.x * 8) + (18 * ui_scale);
			int time_width = (dot_size.x * 3) + (digit_size.x * 12) + (24 * ui_scale);

			//Draw backgrounds
			ImVec2 start_position = ImGui::GetCursorScreenPos();
			ImDrawList* draw_list = ImGui::GetWindowDrawList();
			draw_list->AddRectFilled(ImVec2(start_position.x, start_position.y),
				ImVec2(start_position.x + date_width, start_position.y + slash_size.y + (6 * ui_scale)), input_background, style.ChildRounding);
			draw_list->AddRectFilled(ImVec2(start_position.x + date_width + (10 * ui_scale), start_position.y),
				ImVec2(start_position.x + date_width + (10 * ui_scale) + time_width, start_position.y + slash_size.y + (6 * ui_scale)), input_background, style.ChildRounding);

			//Date
			ImGui::SetCursorPosX(start_position.x + (3 * ui_scale));
			ImGui::PushItemWidth(digit_size.x * 4);
			if (ImGui::InputInt("/###dsparamyear", &year_holder, 0, 0, ImGuiInputTextFlags_NoHorizontalScroll))
				timestamp->tm_year = year_holder - 1900;
			ImGui::PopItemWidth();
			ImGui::SameLine();
			ImGui::PushItemWidth(digit_size.x * 2);
			if (ImGui::InputScalar("/###dsparammonth", ImGuiDataType_S32, (void*)&month_holder, (void*)NULL, (void*)NULL, "%02d", ImGuiInputTextFlags_NoHorizontalScroll))
			{
				if (month_holder > 12)
					month_holder = 12;
				if (month_holder < 1)
					month_holder = 1;
				timestamp->tm_mon = month_holder - 1;
			}
			ImGui::SameLine();
			if (ImGui::InputScalar("###dsparamday", ImGuiDataType_S32, (void*)&timestamp->tm_mday, (void*)NULL, (void*)NULL, "%02d", ImGuiInputTextFlags_NoHorizontalScroll))
			{
				if (timestamp->tm_mday > 31 || timestamp->tm_mday < 1)
					timestamp->tm_mday = 1;
			}
			ImGui::SameLine(0.0, 16.0 * ui_scale);

			//Time
			if (ImGui::InputScalar(":###dsparamhour", ImGuiDataType_S32, (void*)&timestamp->tm_hour, (void*)NULL, (void*)NULL, "%02d", ImGuiInputTextFlags_NoHorizontalScroll))
			{
				if (timestamp->tm_hour > 23)
					timestamp->tm_hour = 23;
				if (timestamp->tm_hour < 0)
					timestamp->tm_hour = 0;
			}
			ImGui::SameLine();
			if (ImGui::InputScalar(":###dsparammin", ImGuiDataType_S32, (void*)&timestamp->tm_min, (void*)NULL, (void*)NULL, "%02d", ImGuiInputTextFlags_NoHorizontalScroll))
			{
				if (timestamp->tm_min > 59)
					timestamp->tm_min = 59;
				if (timestamp->tm_min < 0)
					timestamp->tm_min = 0;
			}
			ImGui::SameLine();
			if (ImGui::InputScalar(".###dsparamsec", ImGuiDataType_S32, (void*)&timestamp->tm_sec, (void*)NULL, (void*)NULL, "%02d", ImGuiInputTextFlags_NoHorizontalScroll))
			{
				if (timestamp->tm_sec > 59)
					timestamp->tm_sec = 59;
				if (timestamp->tm_sec < 0)
					timestamp->tm_sec = 0;
			}
			ImGui::PopItemWidth();
			ImGui::SameLine();
			ImGui::PushItemWidth(digit_size.x * 6);
			ImGui::InputText("###dsparamsecdec", &seconds_decimal, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_NoHorizontalScroll);
			ImGui::PopItemWidth();
			ImGui::SameLine();
			ImGui::PopStyleVar(3);
			ImGui::PopStyleColor();
			ImGui::TextUnformatted(" UTC");
		}
	}
} // namespace widgets
