#include "format_notated.h"
#include <cstdint>
#include <sstream>
#include <iomanip>
#include <vector>
#include <type_traits>

template <typename T>
std::string format_notated(T val, std::string units, int num_decimals)
{
    std::ostringstream render_stream;
    double display_double;
    std::string display_suffix;
    T abs_val;
    bool no_units = (units == "");
    std::string format_spacing = no_units ? "" : " ";

    if constexpr (std::is_unsigned<T>::value)
        abs_val = val;
    else
        abs_val = std::abs(val);

    if (abs_val < 1e-6)
    {
        display_double = val / 1e-9;
        display_suffix = format_spacing + "n" + units;
    }
    else if (abs_val < 1e-3)
    {
        display_double = val / 1e-6;
        display_suffix = format_spacing + "µ" + units;
    }
    else if (abs_val < 1e0)
    {
        display_double = val / 1e-3;
        display_suffix = format_spacing + "m" + units;
    }
    else if (abs_val < 1e3)
    {
        display_double = val;
        display_suffix = " " + units;
    }
    else if (abs_val < (no_units ? 1e7 : 1e6))
    {
        display_double = (double)val / 1e3;
        display_suffix = format_spacing + "k" + units;
    }
    else if (abs_val < (no_units ? 1e10 : 1e9))
    {
        display_double = (double)val / 1e6;
        display_suffix = format_spacing + "M" + units;
    }
    else if (abs_val < 1e12)
    {
        display_double = (double)val / 1e9;
        display_suffix = format_spacing + "G" + units;
    }
    else if (abs_val < 1e15)
    {
        display_double = (double)val / 1e12;
        display_suffix = format_spacing + "T" + units;
    }
    else
    {
        display_double = (double)val / 1e15;
        display_suffix = format_spacing + "P" + units;
    }

    if (num_decimals >= 0)
        render_stream << std::setprecision(num_decimals) << std::fixed;

    render_stream << display_double << display_suffix;
    return render_stream.str();
}

template std::string format_notated(uint64_t, std::string, int);
template std::string format_notated(int64_t, std::string, int);
template std::string format_notated(int, std::string, int);
template std::string format_notated(float, std::string, int);
template std::string format_notated(double, std::string, int);
