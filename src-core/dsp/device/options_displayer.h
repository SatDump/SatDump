#pragma once

#include <mutex>
#include "nlohmann/json.hpp"
#include "imgui/imgui.h"
#include "imgui/imgui_stdlib.h"
#include "common/widgets/stepped_slider.h"
#include "core/exception.h"

namespace satdump
{
    namespace todorework // rename!!!!! And move to somewhere more generic!!!! And document!!!
    {
        class OptionsDisplayer
        {
        private:
            struct OptHolder
            {
                // ID
                std::string id;
                std::string name;

                // Type
                bool is_bool = false;
                bool is_string = false;
                bool is_uint = false;
                bool is_int = false;
                bool is_float = false;
                bool is_sub = false;

                bool is_list = false;
                bool is_range = false;

                // Value config. TODOREWORK, handle lists as uint64_t too?
                std::vector<double> list;
                std::vector<std::string> list_string;
                std::array<double, 3> range;

                // Holding values
                bool _bool = false;
                std::string _string = "";
                uint64_t _uint = 0;
                int64_t _int = 0;
                double _float = 0;
                std::shared_ptr<OptionsDisplayer> _sub;
            };

            std::mutex opts_mtx;
            std::vector<OptHolder> opts;

        public:
            void add_options(nlohmann::json options)
            {
                opts_mtx.lock();
                for (auto &v : options.items())
                {
                    auto &vv = v.value();
                    OptHolder h;
                    h.id = v.key();

                    // Validate and parse config
                    if (!vv.contains("type"))
                        throw satdump_exception("Option " + v.key() + " does not have type!");

                    if (vv.contains("name"))
                        h.name = vv["name"];
                    else
                        h.name = h.id;

                    h.is_bool = vv["type"] == "bool";
                    h.is_string = vv["type"] == "string";
                    h.is_uint = vv["type"] == "uint";
                    h.is_int = vv["type"] == "int";
                    h.is_float = vv["type"] == "float";
                    h.is_sub = vv["type"] == "sub";

                    if (h.is_sub)
                    {
                        h._sub = std::make_shared<OptionsDisplayer>();
                        h._sub->add_options(vv["sub"]);
                    }

                    h.is_list = vv.contains("list");
                    if (h.is_list)
                    {
                        if (h.is_string)
                            h.list_string = vv["list"].get<std::vector<std::string>>();
                        else
                            h.list = vv["list"].get<std::vector<double>>();
                    }

                    h.is_range = vv.contains("range");
                    if (h.is_range)
                        h.range = vv["range"].get<std::array<double, 3>>();

                    opts.push_back(h);
                }
                opts_mtx.unlock();
            }

            void clear()
            {
                opts_mtx.lock();
                opts.clear();
                opts_mtx.unlock();
            }

            void set_values(nlohmann::json vals)
            {
                opts_mtx.lock();
                for (auto &v : opts)
                {
                    if (vals.contains(v.id))
                    {
                        auto &j = vals[v.id];
                        if (v.is_bool && j.is_boolean())
                            v._bool = j;
                        else if (v.is_string && j.is_string())
                            v._string = j;
                        else if (v.is_uint && j.is_number())
                            v._uint = j;
                        else if (v.is_int && j.is_number())
                            v._int = j;
                        else if (v.is_float && j.is_number())
                            v._float = j;
                        else if (v.is_sub && j.is_object())
                            v._sub->set_values(j);
                        else
                            throw satdump_exception("Invalid options or JSON value! (SET " + v.id + ")");
                    }
                }
                opts_mtx.unlock();
            }

            nlohmann::json get_values()
            {
                opts_mtx.lock();
                nlohmann::json vals;
                for (auto &v : opts)
                {
                    auto &j = vals[v.id];
                    if (v.is_bool)
                        j = v._bool;
                    else if (v.is_string)
                        j = v._string;
                    else if (v.is_uint)
                        j = v._uint;
                    else if (v.is_int)
                        j = v._int;
                    else if (v.is_float)
                        j = v._float;
                    else if (v.is_sub)
                        j = v._sub->get_values();
                    else
                        throw satdump_exception("Invalid options or JSON value! (GET)");
                }
                opts_mtx.unlock();
                return vals;
            }

            bool draw()
            {
                bool u = false;
                opts_mtx.lock();
                for (auto &v : opts)
                {
                    std::string id = v.name; // TODOREWORK might cause issues? std::string(v.name + "##optsdisplayertodorework");

                    if (v.is_bool)
                    {
                        u |= ImGui::Checkbox(id.c_str(), &v._bool);
                    }
                    else if (v.is_list && v.is_string)
                    {
                        if (ImGui::BeginCombo(id.c_str(), v._string.c_str()))
                        {
                            for (auto &i : v.list_string)
                                if (ImGui::Selectable(i.c_str(), i == v._string))
                                    u = true, v._string = i;
                            ImGui::EndCombo();
                        }
                    }
                    else if (v.is_string)
                    {
                        u |= ImGui::InputText(id.c_str(), &v._string);
                    }
                    else if (v.is_range && v.is_int)
                    {
                        // TODOREWORK ALLOW LARGE INT
                        int lv = v._int;
                        u |= widgets::SteppedSliderInt(id.c_str(), &lv, v.range[0], v.range[1], v.range[2]);
                        v._int = lv;
                    }
                    else if (v.is_range && v.is_uint)
                    {
                        // TODOREWORK ALLOW LARGE INT
                        int lv = v._uint;
                        u |= widgets::SteppedSliderInt(id.c_str(), &lv, v.range[0], v.range[1], v.range[2]);
                        v._uint = lv;
                    }
                    else if (v.is_range && v.is_float)
                    {
                        // TODOREWORK USE DOUBLE INSTEAD
                        float lv = v._float;
                        u |= widgets::SteppedSliderFloat(id.c_str(), &lv, v.range[0], v.range[1], v.range[2]);
                        v._float = lv;
                    }
                    else if (v.is_list && v.is_int)
                    {
                        // TODOREWORK USE LARGE INT INSTEAD
                        if (ImGui::BeginCombo(id.c_str(), std::to_string(v._int).c_str()))
                        {
                            for (auto &i : v.list)
                                if (ImGui::Selectable(std::to_string(i).c_str(), i == v._int))
                                    u = true, v._int = i;
                            ImGui::EndCombo();
                        }
                    }
                    else if (v.is_list && v.is_uint)
                    {
                        // TODOREWORK USE LARGE INT INSTEAD
                        if (ImGui::BeginCombo(id.c_str(), std::to_string(v._uint).c_str()))
                        {
                            for (auto &i : v.list)
                                if (ImGui::Selectable(std::to_string(i).c_str(), i == v._uint))
                                    u = true, v._uint = i;
                            ImGui::EndCombo();
                        }
                    }
                    else if (v.is_list && v.is_float)
                    {
                        // TODOREWORK USE DOUBLE INSTEAD
                        if (ImGui::BeginCombo(id.c_str(), std::to_string(v._float).c_str()))
                        {
                            for (auto &i : v.list)
                                if (ImGui::Selectable(std::to_string(i).c_str(), i == v._float))
                                    u = true, v._float = i;
                            ImGui::EndCombo();
                        }
                    }
                    else if (v.is_int)
                    {
                        // TODOREWORK USE LARGE INT INSTEAD
                        int lv = v._int;
                        u |= ImGui::InputInt(id.c_str(), &lv);
                        v._int = lv;
                    }
                    else if (v.is_uint)
                    {
                        // TODOREWORK USE LARGE INT INSTEAD
                        int lv = v._uint;
                        u |= ImGui::InputInt(id.c_str(), &lv);
                        v._uint = lv;
                    }
                    else if (v.is_float)
                    {
                        // TODOREWORK USE DOUBLE INSTEAD
                        u |= ImGui::InputDouble(id.c_str(), &v._float);
                    }
                    else if (v.is_sub)
                    {
                        // TODOREWORK? Check

                        ImGui::Separator();
                        ImGui::Text("%s", id.c_str());
                        ImGui::Separator();
                        u |= v._sub->draw();
                        ImGui::Separator();
                    }
                    else
                    {
                        ImGui::Text("Unimplemented : %s", id.c_str());
                    }
                }
                opts_mtx.unlock();
                return u;
            }
        };
    }
}