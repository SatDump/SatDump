#include "double_list.h"
#include "imgui/imgui.h"

namespace widgets
{
    DoubleList::DoubleList(std::string name)
    {
        d_id = name + "##id" + std::to_string(rand());
        d_id_man = "Manual " + name + "##id" + std::to_string(rand());
    }

    bool DoubleList::render()
    {
        bool v = ImGui::Combo(d_id.c_str(), &selected_value, values_option_str.c_str());

        if (allow_manual && selected_value == (int)available_values.size() - 1)
            v = v || ImGui::InputDouble(d_id_man.c_str(), &current_value, 10e3, 100e3, "%.0f");
        else
            current_value = available_values[selected_value];

        return v;
    }

    void DoubleList::set_list(std::vector<double> list, bool allow_manual, std::function<std::string(double v)> format_func)
    {
        this->allow_manual = allow_manual;

        available_values.clear();
        values_option_str.clear();

        available_values = list;

        values_option_str = "";
        for (double &v : available_values)
            values_option_str += format_func(v) + '\0';

        if (allow_manual)
        {
            available_values.push_back(-1);
            values_option_str += "Manual" + '\0';
        }
    }

    double DoubleList::get_value()
    {
        if (allow_manual && selected_value == (int)available_values.size() - 1)
            ;
        else
            current_value = available_values[selected_value];

        return current_value;
    }

    bool DoubleList::set_value(double v, double manual_max)
    {
        for (int i = 0; i < (int)available_values.size(); i++)
        {
            if (v == available_values[i])
            {
                selected_value = i;
                current_value = v;
                return true;
            }
        }

        if (allow_manual && manual_max != 0 && v <= manual_max)
        {
            selected_value = (int)available_values.size() - 1;
            current_value = v;
            return true;
        }

        return false;
    }
}