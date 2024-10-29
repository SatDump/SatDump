#include "imgui/imgui.h"
#include "imgui/imgui_stdlib.h"
#include "logger.h"
#include "params.h"

namespace satdump
{
    namespace params
    {
        EditableParameter::EditableParameter(nlohmann::json p_json)
        {
            d_imgui_id = rand();
            std::string type_str = p_json["type"].get<std::string>();
            bool hasValue = p_json.count("value") > 0;
            d_name = p_json["name"].get<std::string>();
            d_id = /*d_name +*/ "##" + std::to_string(d_imgui_id);
            if (p_json.count("description") > 0)
                d_description = p_json["description"].get<std::string>();

            if (type_str == "string")
            {
                d_type = PARAM_STRING;
                if (hasValue)
                    p_string = p_json["value"].get<std::string>();
                else
                    p_string = "";
            }
            else if (type_str == "password")
            {
                d_type = PARAM_PASSWORD;
                if (hasValue)
                    p_string = p_json["value"].get<std::string>();
                else
                    p_string = "";
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
                for (std::string &opt : d_options)
                    d_options_str += opt + '\0';

                if (hasValue)
                {
                    int i = 0;
                    for (std::string &opt : d_options)
                    {
                        if (p_json["value"].get<std::string>() == opt)
                            d_option = i;
                        i++;
                    }
                }
                else
                    d_option = 0;
            }
            else if (type_str == "path")
            {
                d_type = PARAM_PATH;

                file_select = std::make_shared<FileSelectWidget>(p_json["name"], p_json["name"], p_json["is_directory"]);
                file_select->setPath(p_json["value"]);
            }
            else if (type_str == "timestamp")
            {
                d_type = PARAM_TIMESTAMP;
                date_time_picker = std::make_shared<widgets::DateTimePicker>(d_id, hasValue ? p_json["value"].get<double>() : -1);
            }
            else if (type_str == "notated_int")
            {
                d_type = PARAM_NOTATED_INT;
                std::string units;
                if (p_json.count("units") > 0)
                    units = p_json["units"].get<std::string>();
                else
                    units = "";

                notated_int = std::make_shared<widgets::NotatedNum<int64_t>>(d_id, hasValue ? p_json["value"].get<int64_t>() : 0, units);
            }
            else if (type_str == "color")
            {
                d_type = PARAM_COLOR;
                std::vector<float> color = p_json["value"].get<std::vector<float>>();
                p_color[0] = color[0];
                p_color[1] = color[1];
                p_color[2] = color[2];
            }
            else if (type_str == "baseband_type")
            {
                d_type = PARAM_BASEBAND_TYPE;
                baseband_type = p_json["value"].get<std::string>();
            }
            else if (type_str == "labeled_options")
            {
                d_type = PARAM_LABELED_OPTIONS;

                d_labeled_opts = p_json["options"];
                p_bool = p_json.contains("manual") ? p_json["manual"].get<bool>() : false;

                d_options_str = "";
                for (std::pair<std::string, std::string> &opt : d_labeled_opts)
                    d_options_str += opt.second + '\0';

                if(p_bool) // Allow manual
                    d_options_str += std::string("Custom") + '\0';

                if (hasValue)
                {
                    size_t i = 0;
                    p_string = p_json["value"].get<std::string>();
                    for (std::pair<std::string, std::string> &opt : d_labeled_opts)
                    {
                        if (p_json["value"].get<std::string>() == opt.first)
                        {
                            d_option = i;
                            break;
                        }
                        i++;
                    }
                    if (i == d_labeled_opts.size())
                    {
                        if(p_bool) // Allow Manual
                            d_option = i;
                        else
                            d_option = 0;
                    }
                }
                else
                {
                    d_option = 0;
                    p_string = d_labeled_opts[0].first;
                }
            }
            else
            {
                logger->error("Invalid options on EditableParameter!");
            }
        }

        void EditableParameter::draw()
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", d_name.c_str());
            if (ImGui::IsItemHovered() && d_description.size() > 0)
                ImGui::SetTooltip("%s", d_description.c_str());
            ImGui::TableSetColumnIndex(1);

            if (d_type == PARAM_STRING)
                ImGui::InputText(d_id.c_str(), &p_string);
            else if (d_type == PARAM_PASSWORD)
                ImGui::InputText(d_id.c_str(), &p_string, ImGuiInputTextFlags_Password);
            else if (d_type == PARAM_INT)
                ImGui::InputInt(d_id.c_str(), &p_int, 0);
            else if (d_type == PARAM_FLOAT)
                ImGui::InputDouble(d_id.c_str(), &p_float);
            else if (d_type == PARAM_BOOL)
                ImGui::Checkbox(d_id.c_str(), &p_bool);
            else if (d_type == PARAM_OPTIONS)
                ImGui::Combo(d_id.c_str(), &d_option, d_options_str.c_str());
            else if (d_type == PARAM_PATH)
                file_select->draw();
            else if (d_type == PARAM_TIMESTAMP)
                date_time_picker->draw();
            else if (d_type == PARAM_NOTATED_INT)
                notated_int->draw();
            else if (d_type == PARAM_COLOR)
                ImGui::ColorEdit3(d_id.c_str(), (float *)p_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
            else if (d_type == PARAM_BASEBAND_TYPE)
                baseband_type.draw_playback_combo(d_id.c_str());
            else if (d_type == PARAM_LABELED_OPTIONS)
            {
                if (ImGui::Combo(d_id.c_str(), &d_option, d_options_str.c_str()) && d_option != (int)d_labeled_opts.size())
                    p_string = d_labeled_opts[d_option].first;

                if (p_bool) // Allow Manual
                {
                    if (d_option != (int)d_labeled_opts.size())
                        ImGui::BeginDisabled();
                    ImGui::InputText(std::string(d_id + "_custom").c_str(), &p_string);
                    if (d_option != (int)d_labeled_opts.size())
                        ImGui::EndDisabled();
                }
            }
        }

        nlohmann::json EditableParameter::getValue()
        {
            nlohmann::json v;
            if (d_type == PARAM_STRING || d_type == PARAM_PASSWORD || d_type == PARAM_LABELED_OPTIONS)
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
            else if (d_type == PARAM_TIMESTAMP)
                v = date_time_picker->get();
            else if (d_type == PARAM_NOTATED_INT)
                v = notated_int->get();
            else if (d_type == PARAM_COLOR)
                v = {p_color[0], p_color[1], p_color[2]};
            else if (d_type == PARAM_BASEBAND_TYPE)
                v = (std::string)baseband_type;
            return v;
        }

        nlohmann::json EditableParameter::setValue(nlohmann::json v)
        {
            if (d_type == PARAM_STRING || d_type == PARAM_PASSWORD)
                p_string = v.get<std::string>();
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
                file_select->setPath(v.get<std::string>());
            else if (d_type == PARAM_TIMESTAMP)
                date_time_picker->set(v.get<double>());
            else if (d_type == PARAM_NOTATED_INT)
                notated_int->set(v.get<uint64_t>());
            else if (d_type == PARAM_COLOR)
            {
                std::vector<float> color = v.get<std::vector<float>>();
                p_color[0] = color[0];
                p_color[1] = color[1];
                p_color[2] = color[2];
            }
            else if (d_type == PARAM_BASEBAND_TYPE)
                baseband_type = v.get<std::string>();
            else if (d_type == PARAM_LABELED_OPTIONS)
            {
                size_t i = 0;
                p_string = v.get<std::string>();
                for (std::pair<std::string, std::string> &opt : d_labeled_opts)
                {
                    if (p_string == opt.first)
                    {
                        d_option = i;
                        break;
                    }
                    i++;
                }
                if (i == d_labeled_opts.size() && p_bool) // Allow manual
                    d_option = i;
            }
            return v;
        }
    }
}