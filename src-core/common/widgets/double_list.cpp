#include "double_list.h"
#include "common/rimgui.h"
#include "common/dsp_source_sink/format_notated.h"

namespace widgets
{
    DoubleList::DoubleList(std::string name)
    {
        d_id = name + "##id" + std::to_string(rand());
        current_value = new NotatedNum<double>("Manual " + name + "##id" + std::to_string(rand()), 0, "sps");
    }

    DoubleList::~DoubleList()
    {
        delete current_value;
    }

    bool DoubleList::render()
    {
        bool v = RImGui::Combo(d_id.c_str(), &selected_value, values_option_str.c_str());

        if (allow_manual && selected_value == (int)available_values.size() - 1)
            v = v || current_value->draw();
        else
            current_value->set(available_values[selected_value]);

        return v;
    }

    void DoubleList::set_list(std::vector<double> list, bool allow_manual, std::string units)
    {
        this->allow_manual = allow_manual;

        available_values.clear();
        values_option_str = "";

        available_values = list;

        for (double &v : available_values)
            values_option_str += format_notated(v, units) + '\0';

        if (allow_manual)
        {
            available_values.push_back(-1);
            values_option_str += "Manual";
            values_option_str += '\0';
        }
    }

    double DoubleList::get_value()
    {
        if (!allow_manual || selected_value != (int)available_values.size() - 1)
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

        if (allow_manual && manual_max != 0 && v <= manual_max)
        {
            selected_value = (int)available_values.size() - 1;
            current_value->set(v);
            return true;
        }

        return false;
    }
}
