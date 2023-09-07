#include <sstream>
#include <regex>
#include "imgui/imgui.h"
#include "imgui/imgui_stdlib.h"
#include "logger.h"
#include "notated_int.h"

namespace widgets
{
    NotatedInt::NotatedInt(std::string d_id, int64_t input_int, std::string units) : val{ input_int }, d_id{ d_id }, units{ units }
    {
        render_int();
    }
    NotatedInt::~NotatedInt()
    {
    }
    void NotatedInt::parse_input()
    {
        //Clean up string
        display_val.erase(std::remove_if(display_val.begin(), display_val.end(), ::isspace), display_val.end());
        std::regex regexp(units, std::regex_constants::icase);
        display_val = std::regex_replace(display_val, regexp, "");

        //Get order of magnitude suffix, if present
        uint64_t multiplier = 1;
        bool had_suffix = true;
        const char suffix = toupper(display_val.back());
        if (suffix == 'K')
            multiplier = 1e3;
        else if (suffix == 'M')
            multiplier = 1e6;
        else if (suffix == 'G')
            multiplier = 1e9;
        else if (suffix == 'T')
            multiplier = 1e12;
        else if (suffix == 'P')
            multiplier = 1e15;
        else
            had_suffix = false;

        //Remove suffix
        if(had_suffix)
            display_val.pop_back();

        //Try to parse the rest of it as a number
        uint64_t decoded_int;
        try
        {
            decoded_int = std::stod(display_val) * multiplier;
        }
        catch (std::exception &e)
        {
            logger->trace("Ignoring bad int input - " + std::string(e.what()));
            display_val = last_display;
            return;
        }

        //Value is good - update and re-render
        val = decoded_int;
        render_int();
    }
    void NotatedInt::render_int()
    {
        std::ostringstream render_stream;
        double display_double;
        std::string display_suffix;
        int64_t abs_val = abs(val);

        if (abs_val < 1e3)
        {
            display_double = val;
            display_suffix = " " + units;
        }
        else if (abs_val < 1e6)
        {
            display_double = (double)val / 1e3;
            display_suffix = " k" + units;
        }
        else if (abs_val < 1e9)
        {
            display_double = (double)val / 1e6;
            display_suffix = " M" + units;
        }
        else if (abs_val < 1e12)
        {
            display_double = (double)val / 1e9;
            display_suffix = " G" + units;
        }
        else if (abs_val < 1e15)
        {
            display_double = (double)val / 1e12;
            display_suffix = " T" + units;
        }
        else
        {
            display_double = (double)val / 1e15;
            display_suffix = " P" + units;
        }

        render_stream << display_double << display_suffix;
        last_display = display_val = render_stream.str();
    }
    void NotatedInt::draw()
    {
        ImGui::InputText(d_id.c_str(), &display_val, ImGuiInputTextFlags_AutoSelectAll);
        if(ImGui::IsItemDeactivatedAfterEdit())
            parse_input();
    }
    int NotatedInt::get()
    {
        return val;
    }
    void NotatedInt::set(uint64_t input_int)
    {
        val = input_int;
        render_int();
    }
}