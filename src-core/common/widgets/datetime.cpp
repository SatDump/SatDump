#include "datetime.h"
#include "imgui/imgui_stdlib.h"

#ifdef _WIN32
#define timegm _mkgmtime
#endif

namespace widgets
{
	DateTimePicker::DateTimePicker(float input_time)
	{
		ImGuiStyle style = ImGui::GetStyle();
		item_spacing_y = style.ItemSpacing.y;
		item_inner_spacing_y = style.ItemInnerSpacing.y;
		frame_padding_y = style.FramePadding.y;

		handle_input(input_time);
	}
	DateTimePicker::~DateTimePicker()
	{
	}
	void DateTimePicker::handle_input(float input_time)
	{
		time_t input_timet;
		if (input_time <= 0)
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
			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0, 0.0, 0.0, 0.0));
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, frame_padding_y));
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(3, item_spacing_y));
			ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(3, item_inner_spacing_y));

			//Date
			ImGui::PushItemWidth(ImGui::CalcTextSize(std::to_string(year_holder).c_str()).x);
			if (ImGui::InputInt("/###dsparamyear", &year_holder, 0))
				timestamp->tm_year = year_holder - 1900;
			ImGui::PopItemWidth();
			ImGui::SameLine();
			ImGui::PushItemWidth(ImGui::CalcTextSize(std::to_string(month_holder).c_str()).x);
			if (ImGui::InputInt("/###dsparammonth", &month_holder, 0))
			{
				if (month_holder > 12)
					month_holder = 12;
				if (month_holder < 1)
					month_holder = 1;
				timestamp->tm_mon = month_holder - 1;
			}
			ImGui::PopItemWidth();
			ImGui::SameLine();
			ImGui::PushItemWidth(ImGui::CalcTextSize(std::to_string(timestamp->tm_mday).c_str()).x);
			if (ImGui::InputInt("###dsparamday", &timestamp->tm_mday, 0))
			{
				if (timestamp->tm_mday > 31 || timestamp->tm_mday < 1)
					timestamp->tm_mday = 1;
			}
			ImGui::PopItemWidth();
			ImGui::SameLine(0.0, 20.0);

			//Time
			ImGui::PushItemWidth(ImGui::CalcTextSize(std::to_string(timestamp->tm_hour).c_str()).x);
			if (ImGui::InputInt(":###dsparamhour", &timestamp->tm_hour, 0))
			{
				if (timestamp->tm_hour > 23)
					timestamp->tm_hour = 23;
				if (timestamp->tm_hour < 0)
					timestamp->tm_hour = 0;
			}
			ImGui::PopItemWidth();
			ImGui::SameLine();
			ImGui::PushItemWidth(ImGui::CalcTextSize(std::to_string(timestamp->tm_min).c_str()).x);
			if (ImGui::InputInt(":###dsparammin", &timestamp->tm_min, 0))
			{
				if (timestamp->tm_min > 59)
					timestamp->tm_min = 59;
				if (timestamp->tm_min < 0)
					timestamp->tm_min = 0;
			}
			ImGui::PopItemWidth();
			ImGui::SameLine();
			ImGui::PushItemWidth(ImGui::CalcTextSize(std::to_string(timestamp->tm_sec).c_str()).x);
			if (ImGui::InputInt(".###dsparamsec", &timestamp->tm_sec, 0))
			{
				if (timestamp->tm_sec > 59)
					timestamp->tm_sec = 59;
				if (timestamp->tm_sec < 0)
					timestamp->tm_sec = 0;
			}
			ImGui::PopItemWidth();
			ImGui::SameLine();
			ImGui::PushItemWidth(ImGui::CalcTextSize(seconds_decimal.c_str()).x);
			ImGui::InputText("###dsparamsecdec", &seconds_decimal, ImGuiInputTextFlags_CharsDecimal);
			ImGui::PopItemWidth();
			ImGui::SameLine();
			ImGui::PopStyleVar(3);
			ImGui::PopStyleColor();
			ImGui::TextUnformatted(" UTC");
		}
	}
} // namespace widgets
