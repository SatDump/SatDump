#pragma once

#include "imgui/imgui.h"
#include "logger.h"
#include "nlohmann/json.hpp"
#include "imgui/pfd/widget.h"

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
        };

        class EditableParameter
        {
        public:
            ParameterType d_type;
            std::string d_name;
            int d_imgui_id = rand();
            std::string d_id;
            std::string d_description;

            // All the values we might need
            char p_string[1000];

            int p_int;

            double p_float;

            bool p_bool;

            int d_option;
            std::string d_options_str;
            std::vector<std::string> d_options;

            std::shared_ptr<FileSelectWidget> file_select;

        public:
            EditableParameter() {}

            EditableParameter(nlohmann::json p_json)
            {
                std::string type_str = p_json["type"].get<std::string>();
                bool hasValue = p_json.count("value") > 0;
                d_name = p_json["name"].get<std::string>();
                d_id = /*d_name +*/ "##" + std::to_string(d_imgui_id);
                if (p_json.count("description") > 0)
                    d_description = p_json["description"].get<std::string>();

                if (type_str == "string")
                {
                    d_type = PARAM_STRING;
                    memset(p_string, 0, 1000);

                    if (hasValue)
                    {
                        std::string value = p_json["value"].get<std::string>();
                        memcpy(p_string, value.c_str(), std::min<int>(1000, value.size()));
                    }
                }
                else if (type_str == "int")
                {
                    d_type = PARAM_INT;

                    if (hasValue)
                        p_int = p_json["value"].get<int>();
                    else
                        p_int = 0;
                }
                else if (type_str == "float")
                {
                    d_type = PARAM_FLOAT;

                    if (hasValue)
                        p_float = p_json["value"].get<double>();
                    else
                        p_float = 0;
                }
                else if (type_str == "bool")
                {
                    d_type = PARAM_BOOL;

                    if (hasValue)
                        p_bool = p_json["value"].get<bool>();
                    else
                        p_bool = false;
                }
                else if (type_str == "options")
                {
                    d_type = PARAM_OPTIONS;

                    d_options = p_json["options"].get<std::vector<std::string>>();

                    d_options_str = "";
                    for (std::string opt : d_options)
                        d_options_str += opt + '\0';

                    if (hasValue)
                        d_option = p_json["value"].get<int>();
                    else
                        d_option = 0;
                }
                else if (type_str == "path")
                {
                    d_type = PARAM_PATH;

                    file_select = std::make_shared<FileSelectWidget>(p_json["name"], p_json["name"], p_json["is_directory"]);
                    file_select->path = p_json["value"];
                }
                else
                {
                    logger->error("Invalid options on EditableParameter!");
                }
            }

            void draw()
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%s", d_name.c_str());
                if (ImGui::IsItemHovered() && d_description.size() > 0)
                    ImGui::SetTooltip("%s", d_description.c_str());
                ImGui::TableSetColumnIndex(1);

                if (d_type == PARAM_STRING)
                    ImGui::InputText(d_id.c_str(), p_string, 1000);
                else if (d_type == PARAM_INT)
                    ImGui::InputInt(d_id.c_str(), &p_int);
                else if (d_type == PARAM_FLOAT)
                    ImGui::InputDouble(d_id.c_str(), &p_float);
                else if (d_type == PARAM_BOOL)
                    ImGui::Checkbox(d_id.c_str(), &p_bool);
                else if (d_type == PARAM_OPTIONS)
                    ImGui::Combo(d_id.c_str(), &d_option, d_options_str.c_str());
                else if (d_type == PARAM_PATH)
                    file_select->draw();
            }

            nlohmann::json getValue()
            {
                nlohmann::json v;
                if (d_type == PARAM_STRING)
                    v = std::string(p_string);
                else if (d_type == PARAM_INT)
                    v = p_int;
                else if (d_type == PARAM_FLOAT)
                    v = p_float;
                else if (d_type == PARAM_BOOL)
                    v = p_bool;
                else if (d_type == PARAM_OPTIONS)
                    v = d_options[d_option];
                else if (d_type == PARAM_PATH)
                    return file_select->getPath();
                return v;
            }

            nlohmann::json setValue(nlohmann::json v)
            {
                if (d_type == PARAM_STRING)
                {
                    memset(p_string, 0, 1000);
                    std::string value = v.get<std::string>();
                    memcpy(p_string, value.c_str(), std::min<int>(1000, value.size()));
                }
                else if (d_type == PARAM_INT)
                    p_int = v.get<int>();
                else if (d_type == PARAM_FLOAT)
                    p_float = v.get<double>();
                else if (d_type == PARAM_BOOL)
                    p_bool = v.get<bool>();
                else if (d_type == PARAM_OPTIONS)
                {
                    for (int i = 0; i < (int)d_options.size(); i++)
                    {
                        if (d_options[i] == v.get<std::string>())
                            d_option = i;
                    }
                }
                else if (d_type == PARAM_PATH)
                    file_select->path = v.get<std::string>();
                return v;
            }
        };
    }
}