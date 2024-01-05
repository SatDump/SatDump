#pragma once

#include <vector>
#include <string>
#include "notated_num.h"

namespace widgets
{
    class DoubleList
    {
    private:
        bool allow_manual = false;
        int selected_value = 0;
        std::string d_id, values_option_str;
        std::vector<double> available_values;
        NotatedNum<double> *current_value;

    public:
        DoubleList(std::string name);
        ~DoubleList();
        bool render();
        void set_list(std::vector<double> list, bool allow_manual, std::string units = "sps");

        double get_value();
        bool set_value(double v, double manual_max = 0);

        std::vector<double> &get_list() { return available_values; }
    };
}
