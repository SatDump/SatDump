#include "double_list.h"
#include "common/dsp_source_sink/format_notated.h"
#include "common/rimgui.h"

namespace widgets
{
    DoubleList::DoubleList(std::string name)
    {
        d_id = name + "##id" + std::to_string(rand());
        current_value = new NotatedNum<double>("Manual " + name + "##id" + std::to_string(rand()), 0, "sps");
    }

    DoubleList::~DoubleList() { delete current_value; }

    bool DoubleList::render()
    {
        if (available_values.size() == 0)
            return current_value->draw();

        bool v = RImGui::Combo(d_id.c_str(), &selected_value, values_option_str.c_str());
        if (allow_manual && selected_value == (int)available_values.size() - 1)
            v = v || current_value->draw();
        else if (v)
            current_value->set(available_values[selected_value]);

        return v;
    }

    void DoubleList::set_list(std::vector<double> list, bool allow_manual, std::string units)
    {
        this->allow_manual = allow_manual;

        available_values.clear();
        values_option_str = "";
        selected_value = 0;

        available_values = list;

        for (double &v : available_values)
            values_option_str += format_notated(v, units) + '\0';

        if (allow_manual)
        {
            available_values.push_back(-1);
            values_option_str += "Manual";
            values_option_str += '\0';
        }

        current_value->set(available_values[selected_value]);
    }

    double DoubleList::get_value()
    {
        if (!available_values.empty() && (!allow_manual || selected_value != (int)available_values.size() - 1))
            current_value->set(available_values[selected_value]);

        return current_value->get();
    }

    bool DoubleList::set_value(double v, double manual_max)
    {
        for (int i = 0; i < (int)available_values.size(); i++)
        {
            if (v == available_values[i])
            {
                selected_value = i;
                current_value->set(v);
                return true;
            }
        }

        if (allow_manual) // && manual_max != 0 && v <= manual_max) TODOREWORK
        {
            selected_value = (int)available_values.size() - 1;
            current_value->set(v);
            return true;
        }

        return false;
    }
} // namespace widgets
