#pragma once

#include "imgui/imgui.h"
#include "imgui/imgui_stdlib.h"
#include "common/utils.h"
#include <vector>
#include <string>
#include <cstdint>
#include "core/style.h"
#include "dll_export.h"

/*
Super Basic & Crappy remote ImGui Scheme,
but it works?

Need to improve later. Especially clean it up!
*/

namespace RImGui
{
    // Classes/Structs etc
    enum UiElemType
    {
        UI_ELEMENT_NONE,
        UI_ELEMENT_TEXT,
        UI_ELEMENT_BUTTON,
        UI_ELEMENT_RADIOBUTTON,
        UI_ELEMENT_SLIDERINT,
        UI_ELEMENT_SLIDERFLOAT,
        UI_ELEMENT_CHECKBOX,
        UI_ELEMENT_COMBO,
        UI_ELEMENT_INPUTDOUBLE,
        UI_ELEMENT_SEPARATOR,
        UI_ELEMENT_INPUTTEXT,
        UI_ELEMENT_ISITEMDEACTIVATEDAFTEREDIT,
        UI_ELEMENT_SAMELINE,

        UI_ELEMENT_BEGINDISABLED,
        UI_ELEMENT_ENDDISABLED,
    };

    struct UiElem
    {
        UiElemType t = UI_ELEMENT_NONE;
        int id = 0;
        float size_x = 0, size_y = 0;
        /////////////////////
        std::string sv = "";
        int iv = 0;
        bool bv = false;
        float fv = 0;
        //////////////
        double min = 0, max = 0;
        std::string sv2 = "";
        /////////////////////
        bool clicked = false;

        bool operator==(UiElem a)
        {
            return t == a.t &&
                   id == a.id &&
                   size_x == a.size_x &&
                   size_y == a.size_y &&
                   sv == a.sv &&
                   iv == a.iv &&
                   bv == a.bv &&
                   fv == a.fv &&
                   min == a.min &&
                   max == a.max &&
                   sv2 == a.sv2 &&
                   clicked == a.clicked;
        }

        ///////////////////

        int encode(uint8_t *buffer)
        {
            int pos = 0;

            buffer[pos++] = t;

            buffer[pos++] = id >> 8;
            buffer[pos++] = id & 0xFF;

            memcpy(buffer + pos, &size_x, sizeof(size_x));
            pos += 4;
            memcpy(buffer + pos, &size_y, sizeof(size_y));
            pos += 4;

            buffer[pos++] = (sv.size() >> 8) & 0xFF;
            buffer[pos++] = (sv.size() >> 0) & 0xFF;
            for (int i = 0; i < (int)sv.size(); i++)
                buffer[pos++] = sv[i];

            *((int *)&buffer[pos]) = iv;
            pos += 4;

            buffer[pos++] = bv;

            memcpy(buffer + pos, &fv, sizeof(fv));
            pos += 4;

            memcpy(buffer + pos, &min, sizeof(min));
            pos += 8;
            memcpy(buffer + pos, &max, sizeof(max));
            pos += 8;

            buffer[pos++] = (sv2.size() >> 8) & 0xFF;
            buffer[pos++] = (sv2.size() >> 0) & 0xFF;
            for (int i = 0; i < (int)sv2.size(); i++)
                buffer[pos++] = sv2[i];

            buffer[pos++] = clicked;

            return pos;
        }

        int decode(uint8_t *buffer)
        {
            int pos = 0;

            t = (UiElemType)buffer[pos++];

            id = buffer[pos + 0] << 8 | buffer[pos + 1];
            pos += 2;

            memcpy(&size_x, buffer + pos, sizeof(size_x));
            pos += 4;
            memcpy(&size_y, buffer + pos, sizeof(size_y));
            pos += 4;

            int svsize = buffer[pos + 0] << 8 | buffer[pos + 1];
            pos += 2;
            sv.resize(svsize);
            for (int i = 0; i < (int)sv.size(); i++)
                sv[i] = buffer[pos++];

            memcpy(&iv, buffer + pos, sizeof(iv));
            pos += 4;

            bv = buffer[pos++];

            memcpy(&fv, buffer + pos, sizeof(fv));
            pos += 4;

            memcpy(&min, buffer + pos, sizeof(min));
            pos += 8;
            memcpy(&max, buffer + pos, sizeof(max));
            pos += 8;

            int sv2size = buffer[pos + 0] << 8 | buffer[pos + 1];
            pos += 2;
            sv2.resize(sv2size);
            for (int i = 0; i < (int)sv2.size(); i++)
                sv2[i] = buffer[pos++];

            clicked = buffer[pos++];

            return pos;
        }
    };

    inline int encode_vec(uint8_t *buffer, std::vector<UiElem> elems)
    {
        if (elems.size() > 0)
        {
            buffer[0] = (elems.size() >> 8) & 0xFF;
            buffer[1] = (elems.size() >> 0) & 0xFF;
            int pos = 2;
            for (int i = 0; i < (int)elems.size(); i++)
                pos += elems[i].encode(buffer + pos);
            return pos;
        }
        else
        {
            return 0;
        }
    }

    inline std::vector<UiElem> decode_vec(uint8_t *buffer, int len)
    {
        if (len > 2)
        {
            std::vector<UiElem> elems;
            int nel = buffer[0] << 8 | buffer[1];
            elems.resize(nel);
            int pos = 2;
            for (int i = 0; i < (int)elems.size() && pos <= len; i++)
                pos += elems[i].decode(buffer + pos);
            return elems;
        }
        return std::vector<UiElem>();
    }

    struct RImGui
    {
        int current_id = 0;
        std::vector<UiElem> ui_elements;
        std::vector<UiElem> ui_elems_fbk;
        std::vector<UiElem> ui_elems_last;
    };

    // Global variables
    SATDUMP_DLL extern bool is_local;
    SATDUMP_DLL extern RImGui *current_instance;

    // Handlings functions for the "server" rendering
    inline std::vector<UiElem> end_frame(bool force = false)
    {
        std::vector<UiElem> elems; //= ui_elements;

        if (current_instance->ui_elements.size() == current_instance->ui_elems_last.size() && !force)
        {
            bool has_one_not_equal = false;
            for (int i = 0; i < (int)current_instance->ui_elements.size(); i++)
                if (!(current_instance->ui_elems_last[i] == current_instance->ui_elements[i]))
                    has_one_not_equal = true;
            if (has_one_not_equal)
                elems = current_instance->ui_elements;
        }
        else
        {
            elems = current_instance->ui_elements;
        }

        current_instance->ui_elems_last = current_instance->ui_elements;

        current_instance->current_id = 0;
        current_instance->ui_elements.clear();
        current_instance->ui_elems_fbk.clear();

        return elems;
    }

    inline void set_feedback(std::vector<UiElem> fbk)
    {
        current_instance->ui_elems_fbk = fbk;
    }

    // Drawing function "client"
    inline std::vector<UiElem> draw(RImGui *inst, std::vector<UiElem> elems)
    {
        if (elems.size() > 0)
            inst->ui_elems_last = elems;
        else
            elems = inst->ui_elems_last;
        std::vector<UiElem> elems_o = elems;
        for (auto &el : elems)
        {
            if (el.t == UI_ELEMENT_TEXT)
                ImGui::TextUnformatted(el.sv.c_str());
            else if (el.t == UI_ELEMENT_BUTTON)
                el.clicked = ImGui::Button(el.sv.c_str(), {el.size_x, el.size_y});
            else if (el.t == UI_ELEMENT_RADIOBUTTON)
                el.clicked = ImGui::RadioButton(el.sv.c_str(), el.bv);
            else if (el.t == UI_ELEMENT_SLIDERINT)
                el.clicked = ImGui::SliderInt(el.sv.c_str(), &el.iv, el.min, el.max);
            else if (el.t == UI_ELEMENT_SLIDERFLOAT)
                el.clicked = ImGui::SliderFloat(el.sv.c_str(), &el.fv, el.min, el.max);
            else if (el.t == UI_ELEMENT_CHECKBOX)
                el.clicked = ImGui::Checkbox(el.sv.c_str(), &el.bv);
            else if (el.t == UI_ELEMENT_COMBO)
                el.clicked = ImGui::Combo(el.sv.c_str(), &el.iv, el.sv2.c_str());
            else if (el.t == UI_ELEMENT_INPUTDOUBLE)
                el.clicked = ImGui::InputDouble(el.sv.c_str(), &el.min, el.fv, el.max, el.sv2.c_str());
            else if (el.t == UI_ELEMENT_SEPARATOR)
                ImGui::Separator();
            else if (el.t == UI_ELEMENT_INPUTTEXT)
                el.clicked = ImGui::InputText(el.sv.c_str(), &el.sv2, el.iv);
            else if (el.t == UI_ELEMENT_ISITEMDEACTIVATEDAFTEREDIT)
                el.clicked = ImGui::IsItemDeactivatedAfterEdit();
            else if (el.t == UI_ELEMENT_SAMELINE)
                ImGui::SameLine();

            else if (el.t == UI_ELEMENT_BEGINDISABLED)
                style::beginDisabled();
            else if (el.t == UI_ELEMENT_ENDDISABLED)
                style::endDisabled();
        }
        std::vector<UiElem> elems_f;
        for (int i = 0; i < (int)elems_o.size(); i++)
            // if (elems[i].clicked != elems_o[i].clicked ||
            //     elems[i].iv != elems_o[i].iv)
            if (!(elems[i] == elems_o[i]))
                elems_f.push_back(elems[i]);
        return elems_f;
    }

    // Rendering functions, identical to ImGui's
    template <typename... T>
    void Text(const char *fmt, T &&...args)
    {
        if (is_local)
            ImGui::Text(fmt, args...);
        else
            current_instance->ui_elements.push_back({UI_ELEMENT_TEXT,
                                                     current_instance->current_id++,
                                                     0, 0, svformat(fmt, args...)});
    }

    inline bool Button(const char *label, ImVec2 size = ImVec2(0, 0))
    {
        if (is_local)
        {
            return ImGui::Button(label, size);
        }
        else
        {
            current_instance->ui_elements.push_back({UI_ELEMENT_BUTTON,
                                                     current_instance->current_id++,
                                                     size.x, size.y, std::string(label)});
            for (auto &el : current_instance->ui_elems_fbk)
                if (el.t == UI_ELEMENT_BUTTON)
                    if (el.sv == std::string(label))
                        if (el.id == current_instance->current_id - 1)
                            return el.clicked;
            return false;
        }
    }

    inline bool RadioButton(const char *label, bool active)
    {
        if (is_local)
        {
            return ImGui::RadioButton(label, active);
        }
        else
        {
            current_instance->ui_elements.push_back({UI_ELEMENT_RADIOBUTTON,
                                                     current_instance->current_id++,
                                                     0, 0, std::string(label), 0, active});
            for (auto &el : current_instance->ui_elems_fbk)
                if (el.t == UI_ELEMENT_RADIOBUTTON)
                    if (el.sv == std::string(label))
                        if (el.id == current_instance->current_id - 1)
                            return el.clicked;
            return false;
        }
    }

    inline bool SliderInt(const char *label, int *v, int v_min, int v_max)
    {
        if (is_local)
        {
            return ImGui::SliderInt(label, v, v_min, v_max);
        }
        else
        {
            for (auto &el : current_instance->ui_elems_fbk)
                if (el.t == UI_ELEMENT_SLIDERINT)
                    if (el.sv == std::string(label))
                        if (el.id == current_instance->current_id)
                            *v = el.iv;
            current_instance->ui_elements.push_back({UI_ELEMENT_SLIDERINT,
                                                     current_instance->current_id++,
                                                     0, 0, std::string(label), *v, false, 0, (double)v_min, (double)v_max});
            for (auto &el : current_instance->ui_elems_fbk)
                if (el.t == UI_ELEMENT_SLIDERINT)
                    if (el.sv == std::string(label))
                        if (el.id == current_instance->current_id - 1)
                            return el.clicked;
            return false;
        }
    }

    inline bool SliderFloat(const char *label, float *v, float v_min, float v_max)
    {
        if (is_local)
        {
            return ImGui::SliderFloat(label, v, v_min, v_max);
        }
        else
        {
            for (auto &el : current_instance->ui_elems_fbk)
                if (el.t == UI_ELEMENT_SLIDERFLOAT)
                    if (el.sv == std::string(label))
                        if (el.id == current_instance->current_id)
                            *v = el.fv;
            current_instance->ui_elements.push_back({UI_ELEMENT_SLIDERFLOAT,
                                                     current_instance->current_id++,
                                                     0, 0, std::string(label), 0,
                                                     false, *v, v_min, v_max});
            for (auto &el : current_instance->ui_elems_fbk)
                if (el.t == UI_ELEMENT_SLIDERFLOAT)
                    if (el.sv == std::string(label))
                        if (el.id == current_instance->current_id - 1)
                            return el.clicked;
            return false;
        }
    }

    inline bool Checkbox(const char *label, bool *v)
    {
        if (is_local)
        {
            return ImGui::Checkbox(label, v);
        }
        else
        {
            for (auto &el : current_instance->ui_elems_fbk)
                if (el.t == UI_ELEMENT_CHECKBOX)
                    if (el.sv == std::string(label))
                        if (el.id == current_instance->current_id)
                            *v = el.bv;
            current_instance->ui_elements.push_back({UI_ELEMENT_CHECKBOX,
                                                     current_instance->current_id++,
                                                     0, 0, std::string(label), 0, *v});
            for (auto &el : current_instance->ui_elems_fbk)
                if (el.t == UI_ELEMENT_CHECKBOX)
                    if (el.sv == std::string(label))
                        if (el.id == current_instance->current_id - 1)
                            return el.clicked;
            return false;
        }
    }

    inline bool Combo(const char *label, int *current_item, const char *items_separated_by_zeros)
    {
        if (is_local)
        {
            return ImGui::Combo(label, current_item, items_separated_by_zeros);
        }
        else
        {
            for (auto &el : current_instance->ui_elems_fbk)
                if (el.t == UI_ELEMENT_COMBO)
                    if (el.sv == std::string(label))
                        if (el.id == current_instance->current_id)
                            *current_item = el.iv;
            std::string separated_elems;
            {
                const char *p = items_separated_by_zeros; // FIXME-OPT: Avoid computing this, or at least only when combo is open
                while (*p)
                    p += strlen(p) + 1;
                separated_elems.insert(separated_elems.end(), items_separated_by_zeros, p);
            }
            current_instance->ui_elements.push_back({UI_ELEMENT_COMBO,
                                                     current_instance->current_id++,
                                                     0, 0, std::string(label), *current_item,
                                                     false, 0, 0, 0,
                                                     separated_elems});
            for (auto &el : current_instance->ui_elems_fbk)
                if (el.t == UI_ELEMENT_COMBO)
                    if (el.sv == std::string(label))
                        if (el.id == current_instance->current_id - 1)
                            return el.clicked;
            return false;
        }
    }

    inline bool InputDouble(const char *label, double *v, double step = (0.0), double step_fast = (0.0), const char *format = "%.6f")
    {
        if (is_local)
        {
            return ImGui::InputDouble(label, v);
        }
        else
        {
            for (auto &el : current_instance->ui_elems_fbk)
                if (el.t == UI_ELEMENT_INPUTDOUBLE)
                    if (el.sv == std::string(label))
                        if (el.id == current_instance->current_id)
                            *v = el.min;
            current_instance->ui_elements.push_back({UI_ELEMENT_INPUTDOUBLE,
                                                     current_instance->current_id++,
                                                     0, 0, std::string(label), 0,
                                                     false, (float)step, *v, step_fast, std::string(format)});
            for (auto &el : current_instance->ui_elems_fbk)
                if (el.t == UI_ELEMENT_INPUTDOUBLE)
                    if (el.sv == std::string(label))
                        if (el.id == current_instance->current_id - 1)
                            return el.clicked;
            return false;
        }
    }

    inline void Separator()
    {
        if (is_local)
            ImGui::Separator();
        else
            current_instance->ui_elements.push_back({UI_ELEMENT_SEPARATOR,
                                                     current_instance->current_id++,
                                                     0, 0, "##noid"});
    }

    inline bool InputText(const char *label, std::string *str, ImGuiInputTextFlags flags = 0)
    {
        if (is_local)
        {
            return ImGui::InputText(label, str, flags);
        }
        else
        {
            for (auto &el : current_instance->ui_elems_fbk)
                if (el.t == UI_ELEMENT_INPUTTEXT)
                    if (el.sv == std::string(label))
                        if (el.id == current_instance->current_id)
                            *str = el.sv2;
            current_instance->ui_elements.push_back({UI_ELEMENT_INPUTTEXT,
                                                     current_instance->current_id++,
                                                     0, 0, std::string(label), flags,
                                                     false, 0, 0, 0, std::string(*str)});
            for (auto &el : current_instance->ui_elems_fbk)
                if (el.t == UI_ELEMENT_INPUTTEXT)
                    if (el.sv == std::string(label))
                        if (el.id == current_instance->current_id - 1)
                            return el.clicked;
            return false;
        }
    }

    inline bool IsItemDeactivatedAfterEdit()
    {
        if (is_local)
        {
            return ImGui::IsItemDeactivatedAfterEdit();
        }
        else
        {
            current_instance->ui_elements.push_back({UI_ELEMENT_ISITEMDEACTIVATEDAFTEREDIT,
                                                     current_instance->current_id++,
                                                     0, 0, "##nolabelisitemdeactivatedafteredit", 0,
                                                     false, 0, 0, 0, ""});
            for (auto &el : current_instance->ui_elems_fbk)
                if (el.t == UI_ELEMENT_ISITEMDEACTIVATEDAFTEREDIT)
                    if (el.sv == "##nolabelisitemdeactivatedafteredit")
                        if (el.id == current_instance->current_id - 1)
                            return el.clicked;
            return false;
        }
    }

    inline void SameLine()
    {
        if (is_local)
            ImGui::SameLine();
        else
            current_instance->ui_elements.push_back({UI_ELEMENT_SAMELINE,
                                                     current_instance->current_id++,
                                                     0, 0, "##noid"});
    }

    inline void beginDisabled()
    {
        if (is_local)
            style::beginDisabled();
        else
            current_instance->ui_elements.push_back({UI_ELEMENT_BEGINDISABLED,
                                                     current_instance->current_id++});
    }

    inline void endDisabled()
    {
        if (is_local)
            style::endDisabled();
        else
            current_instance->ui_elements.push_back({UI_ELEMENT_ENDDISABLED,
                                                     current_instance->current_id++});
    }
};
