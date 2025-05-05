#include "datetime.h"
#include "imgui/imgui.h"
#include "imgui/imgui_stdlib.h"
#include "core/style.h"
#include <cmath>

#ifdef _WIN32
#define timegm _mkgmtime
#endif

namespace widgets
{
    DateTimePicker::DateTimePicker(std::string id, double input_time, bool date_only)
    {
        start_edit = false;
        d_id = id;
        d_date_only = date_only;
        handle_input(input_time);
    }
    DateTimePicker::~DateTimePicker()
    {
    }
    void DateTimePicker::handle_input(double input_time)
    {
        time_t input_timet;
        std::string seconds_holder;
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
        timestamp = *gmtime(&input_timet);
        seconds_holder = std::to_string(input_time - input_timet);
        seconds_decimal = std::stoi(seconds_holder.substr(seconds_holder.find(".") + 1));
        year_holder = timestamp.tm_year + 1900;
        month_holder = timestamp.tm_mon + 1;
    }
    double DateTimePicker::get()
    {
        if (auto_time)
            return -1.0f;
        else
        {
            double return_value = timegm(&timestamp);
            if (seconds_decimal > 0)
                return_value += ((double)seconds_decimal / std::pow(10.0, int(log10(seconds_decimal) + 1)));

            return return_value;
        }
    }
    void DateTimePicker::set(double input_time)
    {
        handle_input(input_time);
    }
    void DateTimePicker::draw()
    {
        const char* checkbox_label;
        show_picker = auto_time;
        if (auto_time)
            checkbox_label = "Auto###dsauto";
        else
            checkbox_label = "###dsauto";

        ImGui::Checkbox(checkbox_label, &auto_time);

        if (!show_picker)
        {
            //Calculate Draw Size
            ImVec2 slash_size = ImGui::CalcTextSize("/");
            ImVec2 dot_size = ImGui::CalcTextSize(".");
            ImVec2 digit_size = ImGui::CalcTextSize("0");
            int date_width = (slash_size.x * 2) + (digit_size.x * 8) + (18 * ui_scale);
            int time_width = (dot_size.x * 3) + (digit_size.x * 10) + (24 * ui_scale);

            //Setup style
            ImGuiStyle style = ImGui::GetStyle();
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4());
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, style.FramePadding.y));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(3 * ui_scale, style.ItemSpacing.y));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(3 * ui_scale, style.ItemInnerSpacing.y));

            //Draw backgrounds
            ImGui::SameLine();
            ImVec2 start_position = ImGui::GetCursorScreenPos();
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            ImU32 input_background = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_FrameBg]);
            ImU32 input_border = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_Border]);

            draw_list->AddRectFilled(ImVec2(start_position.x, start_position.y),
                ImVec2(start_position.x + date_width, start_position.y + slash_size.y + (6 * ui_scale)), input_background, style.ChildRounding);
            if(!d_date_only)
                draw_list->AddRectFilled(ImVec2(start_position.x + date_width + (10 * ui_scale), start_position.y),
                    ImVec2(start_position.x + date_width + (10 * ui_scale) + time_width, start_position.y + slash_size.y + (6 * ui_scale)), input_background, style.ChildRounding);

            if (style.FrameBorderSize != 0.0f)
            {
                draw_list->AddRect(ImVec2(start_position.x, start_position.y),
                    ImVec2(start_position.x + date_width, start_position.y + slash_size.y + (6 * ui_scale)), input_border, style.ChildRounding);

                if (!d_date_only)
                    draw_list->AddRect(ImVec2(start_position.x + date_width + (10 * ui_scale), start_position.y),
                        ImVec2(start_position.x + date_width + (10 * ui_scale) + time_width, start_position.y + slash_size.y + (6 * ui_scale)), input_border, style.ChildRounding);
            }

            //Date
            bool auto_advance = false;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (3 * ui_scale));
            ImGui::PushItemWidth(digit_size.x * 4);
            if (ImGui::InputInt(std::string("/" + d_id + "year").c_str(), &year_holder, 0, 0, ImGuiInputTextFlags_NoHorizontalScroll))
            {
                timestamp.tm_year = year_holder - 1900;
                if (year_holder > 999)
                    auto_advance = true;
            }
            ImGui::PopItemWidth();
            ImGui::SameLine();

            if (auto_advance)
            {
                ImGui::SetKeyboardFocusHere();
                auto_advance = false;
            }
            ImGui::PushItemWidth(digit_size.x * 2);
            if (ImGui::InputScalar(std::string("/" + d_id + "month").c_str(), ImGuiDataType_S32, (void*)&month_holder, (void*)NULL, (void*)NULL, "%02d", ImGuiInputTextFlags_NoHorizontalScroll))
            {
                if (month_holder == 0)
                    start_edit = true;
                else if (month_holder > 9 || start_edit)
                    auto_advance = true;
            }
            if (ImGui::IsItemClicked() || (ImGui::IsItemFocused() && (ImGui::IsKeyPressed(ImGuiKey_Backspace) || ImGui::IsKeyPressed(ImGuiKey_Delete))))
                start_edit = false;
            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                if (month_holder > 12)
                    month_holder = 12;
                else if (month_holder < 1)
                    month_holder = 1;
                timestamp.tm_mon = month_holder - 1;
                start_edit = false;
            }
            ImGui::SameLine();

            if (auto_advance)
            {
                ImGui::SetKeyboardFocusHere();
                auto_advance = false;
            }
            if (ImGui::InputScalar(std::string(d_id + "day").c_str(), ImGuiDataType_S32, (void*)&timestamp.tm_mday, (void*)NULL, (void*)NULL, "%02d", ImGuiInputTextFlags_NoHorizontalScroll))
            {
                if (timestamp.tm_mday == 0)
                    start_edit = true;
                else if (timestamp.tm_mday > 9 || start_edit)
                    auto_advance = true;
            }
            if (ImGui::IsItemClicked() || (ImGui::IsItemFocused() && (ImGui::IsKeyPressed(ImGuiKey_Backspace) || ImGui::IsKeyPressed(ImGuiKey_Delete))))
                start_edit = false;
            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                if (timestamp.tm_mday > 31 || timestamp.tm_mday < 1)
                    timestamp.tm_mday = 1;
                start_edit = false;
            }
            ImGui::SameLine(0.0, 16.0 * ui_scale);

            //Time
            if (!d_date_only)
            {
                if (auto_advance)
                {
                    ImGui::SetKeyboardFocusHere();
                    auto_advance = false;
                }
                if (ImGui::InputScalar(std::string(":" + d_id + "hour").c_str(), ImGuiDataType_S32, (void*)&timestamp.tm_hour, (void*)NULL, (void*)NULL, "%02d", ImGuiInputTextFlags_NoHorizontalScroll))
                {
                    if (timestamp.tm_hour == 0)
                        start_edit = true;
                    else if (timestamp.tm_hour > 9 || start_edit)
                        auto_advance = true;
                }
                if (ImGui::IsItemClicked() || (ImGui::IsItemFocused() && (ImGui::IsKeyPressed(ImGuiKey_Backspace) || ImGui::IsKeyPressed(ImGuiKey_Delete))))
                    start_edit = false;
                if (ImGui::IsItemDeactivatedAfterEdit())
                {
                    if (timestamp.tm_hour > 23)
                        timestamp.tm_hour = 23;
                    if (timestamp.tm_hour < 0)
                        timestamp.tm_hour = 0;
                    start_edit = false;
                }
                ImGui::SameLine();

                if (auto_advance)
                {
                    ImGui::SetKeyboardFocusHere();
                    auto_advance = false;
                }
                if (ImGui::InputScalar(std::string(":" + d_id + "min").c_str(), ImGuiDataType_S32, (void*)&timestamp.tm_min, (void*)NULL, (void*)NULL, "%02d", ImGuiInputTextFlags_NoHorizontalScroll))
                {
                    if (timestamp.tm_min == 0)
                        start_edit = true;
                    else if (timestamp.tm_min > 9 || start_edit)
                        auto_advance = true;
                }
                if (ImGui::IsItemClicked() || (ImGui::IsItemFocused() && (ImGui::IsKeyPressed(ImGuiKey_Backspace) || ImGui::IsKeyPressed(ImGuiKey_Delete))))
                    start_edit = false;
                if (ImGui::IsItemDeactivatedAfterEdit())
                {
                    if (timestamp.tm_min > 59)
                        timestamp.tm_min = 59;
                    if (timestamp.tm_min < 0)
                        timestamp.tm_min = 0;
                    start_edit = false;
                }
                ImGui::SameLine();

                if (auto_advance)
                {
                    ImGui::SetKeyboardFocusHere();
                    auto_advance = false;
                }
                if (ImGui::InputScalar(std::string("." + d_id + "sec").c_str(), ImGuiDataType_S32, (void*)&timestamp.tm_sec, (void*)NULL, (void*)NULL, "%02d", ImGuiInputTextFlags_NoHorizontalScroll))
                {
                    if (timestamp.tm_sec == 0)
                        start_edit = true;
                    else if (timestamp.tm_sec > 9 || start_edit)
                        auto_advance = true;
                }
                if (ImGui::IsItemClicked() || (ImGui::IsItemFocused() && (ImGui::IsKeyPressed(ImGuiKey_Backspace) || ImGui::IsKeyPressed(ImGuiKey_Delete))))
                    start_edit = false;
                if (ImGui::IsItemDeactivatedAfterEdit())
                {
                    if (timestamp.tm_sec > 59)
                        timestamp.tm_sec = 59;
                    if (timestamp.tm_sec < 0)
                        timestamp.tm_sec = 0;
                    start_edit = false;
                }
                ImGui::PopItemWidth();
                ImGui::SameLine();

                if (auto_advance)
                {
                    ImGui::SetKeyboardFocusHere();
                    auto_advance = false;
                }
                ImGui::PushItemWidth(digit_size.x * 4);
                ImGui::InputInt(std::string(d_id + "dsec").c_str(), &seconds_decimal, 0, 0, ImGuiInputTextFlags_NoHorizontalScroll);
                ImGui::PopItemWidth();
            }
            
            ImGui::SameLine();
            ImGui::PopStyleVar(4);
            ImGui::PopStyleColor();
            ImGui::TextUnformatted(" UTC");
        }
    }
} // namespace widgets
