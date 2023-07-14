#pragma once

#include <vector>
#include <string>
#include <functional>

namespace widgets
{
    class DoubleList
    {
    private:
        bool allow_manual = false;
        int selected_value = 0;
        std::string d_id, d_id_man;
        std::string values_option_str;
        std::vector<double> available_values;
        double current_value = 0;

    public:
        DoubleList(std::string name);
        bool render();
        void set_list(
            std::vector<double> list, bool allow_manual, std::function<std::string(double v)> format_func = [](double v)
                                                         { return std::to_string(v); });

        double get_value();
        bool set_value(double v, double manual_max = 0);
    };
}