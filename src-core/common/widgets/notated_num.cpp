#include <regex>
#include "common/rimgui.h"
#include "common/dsp_source_sink/format_notated.h"
#include "logger.h"
#include "notated_num.h"

namespace widgets
{
    template <typename T>
    NotatedNum<T>::NotatedNum(std::string d_id, T input_int, std::string units) : val{input_int}, d_id{d_id}, units{units}
    {
        last_display = display_val = format_notated(val, units);
    }

    template <typename T>
    NotatedNum<T>::~NotatedNum()
    {
    }

    template <typename T>
    void NotatedNum<T>::parse_input()
    {
        // Clean up string
        display_val.erase(std::remove_if(display_val.begin(), display_val.end(), ::isspace), display_val.end());
        std::regex regexp(units, std::regex_constants::icase);
        display_val = std::regex_replace(display_val, regexp, "");

        // Sanity check
        if (display_val.empty())
        {
            display_val = last_display;
            return;
        }

        // Get order of magnitude suffix, if present
        T multiplier = 1;
        bool had_suffix = true;
        const char suffix = toupper(display_val.back());
        if (suffix == 'K')
            multiplier = 1e3;
        else if (suffix == 'M')
            multiplier = 1e6;
        else if (suffix == 'G')
            multiplier = 1e9;
        else if constexpr (std::is_same<T, int32_t>::value)
            had_suffix = false;
        else
        {
            if (suffix == 'T')
                multiplier = 1e12;
            else if (suffix == 'P')
                multiplier = 1e15;
            else
                had_suffix = false;
        }

        // Remove suffix
        if (had_suffix)
            display_val.pop_back();

        // Try to parse the rest of it as a number
        T decoded_int;
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

        // Value is good - update and re-render
        val = decoded_int;
        last_display = display_val = format_notated(val, units);
    }

    template <typename T>
    bool NotatedNum<T>::draw()
    {
        RImGui::InputText(d_id.c_str(), &display_val, ImGuiInputTextFlags_AutoSelectAll);
        bool retval = RImGui::IsItemDeactivatedAfterEdit();
        if (retval)
            parse_input();

        return retval;
    }

    template <typename T>
    T NotatedNum<T>::get()
    {
        return val;
    }

    template <typename T>
    void NotatedNum<T>::set(T input_int)
    {
        val = input_int;
        last_display = display_val = format_notated(val, units);
    }

    template class NotatedNum<int>;
    template class NotatedNum<uint64_t>;
    template class NotatedNum<int64_t>;
    template class NotatedNum<double>;
}
