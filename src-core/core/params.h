#pragma once
#include "nlohmann/json.hpp"
#include "imgui/pfd/widget.h"
#include "common/widgets/datetime.h"
#include "common/widgets/notated_num.h"

namespace satdump
{
    /*
    This is really mostly means for the user interface,
    to have a common method to represent and input
    parameters, without hardcoding them.
    */
    namespace params
    {
        enum ParameterType
        {
            PARAM_STRING,
            PARAM_INT,
            PARAM_FLOAT,
            PARAM_BOOL,
            PARAM_OPTIONS,
            PARAM_PATH,
            PARAM_TIMESTAMP,
            PARAM_NOTATED_INT,
            PARAM_COLOR,
        };

        class EditableParameter
        {
        public:
            ParameterType d_type;
            std::string d_name;
            int d_imgui_id;
            std::string d_id;
            std::string d_description;

            // All the values we might need
            std::string p_string;
            int p_int;
            double p_float;
            bool p_bool;
            float p_color[4] = {0, 0, 0, 0};
            std::shared_ptr<FileSelectWidget> file_select;
            std::shared_ptr<widgets::DateTimePicker> date_time_picker;
            std::shared_ptr<widgets::NotatedNum<int64_t>> notated_int;

            int d_option;
            std::string d_options_str;
            std::vector<std::string> d_options;

        public:
            EditableParameter(nlohmann::json p_json);
            void draw();
            nlohmann::json getValue();
            nlohmann::json setValue(nlohmann::json v);
        };
    }
}