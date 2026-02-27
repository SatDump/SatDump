#pragma once

#include "common/widgets/stepped_slider.h"
#include "core/exception.h"
#include "core/style.h"
#include "imgui/imgui.h"
#include "imgui/imgui_stdlib.h"
#include "nlohmann/json.hpp"
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>

#include "common/widgets/double_list.h"
#include "common/widgets/frequency_input.h"

#include "logger.h"
#include "nlohmann/json_utils.h"

namespace satdump
{
    namespace ndsp // TODOREWORK rename!!!!! And move to somewhere more generic!!!! And document!!!
    {
        class OptionsDisplayer
        {
        private:
            struct OptHolder
            {
                // ID
                std::string id;
                std::string name;
                bool disable = false;

                // Type
                bool is_bool = false;
                bool is_string = false;
                bool is_uint = false;
                bool is_int = false;
                bool is_float = false;
                bool is_freq = false;
                bool is_samplerate = false;
                bool is_stat = false;

                //                bool is_sub = false;

                bool is_list = false;
                bool is_range = false;
                bool is_range_noslider = false;
                bool is_range_list = false;

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
                std::shared_ptr<widgets::DoubleList> _samplerate;
                nlohmann::json _stat;
            };

            std::mutex opts_mtx;
            std::vector<OptHolder> opts;

            inline void get_val(OptHolder &v, nlohmann::json &j)
            {
                if (v.is_bool)
                    j = v._bool;
                else if (v.is_string)
                    j = v._string;
                else if (v.is_uint || v.is_freq)
                    j = v._uint;
                else if (v.is_int)
                    j = v._int;
                else if (v.is_float)
                    j = v._float;
                else if (v.is_samplerate)
                    j = v._samplerate->get_value();
                else if (v.is_stat)
                    j = v._stat;
                else
                    throw satdump_exception("Invalid options or JSON value! (GET " + v.id + ")");
            }

        public:
            bool show_stats = false;

        public:
            void add_options(nlohmann::ordered_json options)
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

                    // May be hidden!
                    if (vv.contains("hide"))
                        if (vv["hide"].get<bool>())
                            continue;

                    if (vv.contains("disable"))
                        h.disable = vv["disable"].get<bool>();

                    if (vv.contains("name"))
                        h.name = vv["name"];
                    else
                        h.name = h.id;

                    h.is_bool = vv["type"] == "bool";
                    h.is_string = vv["type"] == "string";
                    h.is_uint = vv["type"] == "uint";
                    h.is_int = vv["type"] == "int";
                    h.is_float = vv["type"] == "float";
                    h.is_freq = vv["type"] == "freq";
                    h.is_samplerate = vv["type"] == "samplerate";
                    h.is_stat = vv["type"] == "stat";

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
                    {
                        h.range = vv["range"].get<std::array<double, 3>>();
                        if (vv.contains("range_noslider"))
                            h.is_range_noslider = vv["range_noslider"];
                    }

                    h.is_range_list = h.is_list && h.is_range;

                    if (h.is_samplerate)
                    {
                        h._samplerate = std::make_shared<widgets::DoubleList>(h.name);
                        h._samplerate->set_list(vv["list"], getValueOrDefault(vv["allow_manual"], false));
                    }

                    // Defaults
                    if (vv.contains("default"))
                    {
                        if (h.is_bool)
                            h._bool = vv["default"];
                        else if (h.is_string)
                            h._string = vv["default"];
                        else if (h.is_uint)
                            h._uint = vv["default"];
                        else if (h.is_int)
                            h._int = vv["default"];
                        else if (h.is_float)
                            h._float = vv["default"];
                    }

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
                        else if ((v.is_uint || v.is_freq) && j.is_number())
                            v._uint = j;
                        else if (v.is_int && j.is_number())
                            v._int = j;
                        else if (v.is_float && j.is_number())
                            v._float = j;
                        else if (v.is_samplerate && j.is_number())
                            v._samplerate->set_value(j);
                        else if (v.is_stat)
                            v._stat = j;
                        else
                            logger->error("Invalid options or JSON value! (SET " + v.id + " => " + j.dump() + ")");
                    }
                }
                opts_mtx.unlock();
            }

            nlohmann::json get_values()
            {
                opts_mtx.lock();
                nlohmann::json vals;
                for (auto &v : opts)
                    get_val(v, vals[v.id]);
                opts_mtx.unlock();
                return vals;
            }

            nlohmann::json draw()
            {
                nlohmann::json vals;

                opts_mtx.lock();
                for (auto &v : opts)
                {
                    bool u = false;
                    std::string id = std::string(v.name + "##" + std::to_string((size_t)this));
                    std::string id_n = std::string(v.name);

                    if (v.disable)
                        style::beginDisabled();

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
                        ImGui::InputText(id.c_str(), &v._string);
                        u |= ImGui::IsItemDeactivatedAfterEdit();
                    }
                    else if (v.is_range && v.is_int)
                    {
                        // TODOREWORK ALLOW LARGE INT
                        int lv = v._int;
                        u |= widgets::SteppedSliderInt(id_n.c_str(), &lv, v.range[0], v.range[1], v.range[2]);
                        v._int = lv;
                    }
                    else if (v.is_range && v.is_uint)
                    {
                        // TODOREWORK ALLOW LARGE INT
                        int lv = v._uint;
                        if (v.is_range_noslider)
                        {
                            u |= ImGui::InputInt(id_n.c_str(), &lv);
                            if (lv < v.range[0])
                                lv = v.range[0];
                            if (lv > v.range[1])
                                lv = v.range[1];
                        }
                        else
                            u |= widgets::SteppedSliderInt(id_n.c_str(), &lv, v.range[0], v.range[1], v.range[2]);
                        v._uint = lv;
                    }
                    else if (v.is_range && v.is_float)
                    {
                        // TODOREWORK USE DOUBLE INSTEAD
                        float lv = v._float;
                        u |= widgets::SteppedSliderFloat(id_n.c_str(), &lv, v.range[0], v.range[1], v.range[2]);
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
                        ImGui::InputInt(id.c_str(), &lv);
                        u |= ImGui::IsItemDeactivatedAfterEdit();
                        v._int = lv;
                    }
                    else if (v.is_uint)
                    {
                        // TODOREWORK USE LARGE INT INSTEAD
                        int lv = v._uint;
                        ImGui::InputInt(id.c_str(), &lv);
                        u |= ImGui::IsItemDeactivatedAfterEdit();
                        v._uint = lv;
                    }
                    else if (v.is_float)
                    {
                        // TODOREWORK USE DOUBLE INSTEAD
                        ImGui::InputDouble(id.c_str(), &v._float);
                        u |= ImGui::IsItemDeactivatedAfterEdit();
                    }
                    else if (v.is_freq)
                    {
                        u |= widgets::FrequencyInput(id.c_str(), &v._uint, 0, false);
                        // TODOREWORK handle ranges
                    }
                    else if (v.is_samplerate)
                    {
                        u |= v._samplerate->render();
                    }
                    else if (show_stats && v.is_stat)
                    {
                        ImGui::Text("%s : %s", v.name.c_str(), v._stat.dump().c_str());
                        u = true;
                    }
                    else
                    {
                        ImGui::Text("Unimplemented : %s", id.c_str());
                    }

                    if (v.is_range_list)
                        if (ImGui::Checkbox(("Manual " + v.name).c_str(), &v.is_range))
                            v.is_list = !v.is_range;

                    if (v.disable)
                        style::endDisabled();

                    if (u)
                        get_val(v, vals[v.id]);
                }
                opts_mtx.unlock();
                return vals;
            }
        };
    } // namespace ndsp
} // namespace satdump