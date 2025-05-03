#pragma once
#include "nlohmann/json.hpp"
#include "imgui/dialogs/widget.h"
#include "common/dsp/io/baseband_type.h"
#include "common/widgets/datetime.h"
#include "common/widgets/notated_num.h"

namespace satdump
{
    /*
    This is really mostly means for the user interface,
    to have a common method to represent and input
    parameters, without hardcoding them.
    TODOREWORK DOCUMENT
    */
    namespace params
    {
        class EditableParameter
        {
        private:
            enum ParameterType
            {
                PARAM_STRING,
                PARAM_PASSWORD,
                PARAM_INT,
                PARAM_FLOAT,
                PARAM_BOOL,
                PARAM_OPTIONS,
                PARAM_PATH,
                PARAM_TIMESTAMP,
                PARAM_NOTATED_INT,
                PARAM_COLOR,
                PARAM_BASEBAND_TYPE,
                PARAM_LABELED_OPTIONS
            };

        private:
            ParameterType d_type;
            std::string d_name;
            int d_imgui_id;
            std::string d_id;
            std::string d_description;

        private:
            // All the values we might need
            std::string p_string;
            int p_int;
            double p_float;
            bool p_bool;
            float p_color[4] = {0, 0, 0, 0};
            dsp::BasebandType baseband_type;
            std::shared_ptr<FileSelectWidget> file_select;
            std::shared_ptr<widgets::DateTimePicker> date_time_picker;
            std::shared_ptr<widgets::NotatedNum<int64_t>> notated_int;

            int d_option;
            std::string d_options_str;
            std::vector<std::string> d_options;
            std::vector<std::pair<std::string, std::string>> d_labeled_opts;

        public:
            EditableParameter() {}
            EditableParameter(nlohmann::json p_json);
            void draw();
            nlohmann::json getValue() const;
            nlohmann::json setValue(nlohmann::json v);

        public:
            friend inline void to_json(nlohmann::json &j, const EditableParameter &v)
            {
                std::string type = "invalid";
                if (v.d_type == PARAM_STRING)
                    type = "string";
                else if (v.d_type == PARAM_PASSWORD)
                    type = "password";
                else if (v.d_type == PARAM_LABELED_OPTIONS)
                    type = "labeled_options";
                else if (v.d_type == PARAM_INT)
                    type = "int";
                else if (v.d_type == PARAM_FLOAT)
                    type = "float";
                else if (v.d_type == PARAM_BOOL)
                    type = "bool";
                else if (v.d_type == PARAM_OPTIONS)
                    type = "options";
                else if (v.d_type == PARAM_PATH)
                    type = "path";
                else if (v.d_type == PARAM_TIMESTAMP)
                    type = "timestamp";
                else if (v.d_type == PARAM_NOTATED_INT)
                    type = "notated_int";
                else if (v.d_type == PARAM_COLOR)
                    type = "color";
                else if (v.d_type == PARAM_BASEBAND_TYPE)
                    type = "baseband_type";

                j["name"] = v.d_name;
                if (v.d_description.size() > 0)
                    j["description"] = v.d_description;
                j["value"] = v.getValue();
                j["options"] = v.d_options;
            }

            friend inline void from_json(const nlohmann::json &j, EditableParameter &v)
            {
                v = EditableParameter(j);
            }
        };
    }
}