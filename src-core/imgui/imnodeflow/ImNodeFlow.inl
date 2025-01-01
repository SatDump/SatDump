#pragma once

#include "ImNodeFlow.h"

namespace ImFlow
{
    inline void smart_bezier(const ImVec2& p1, const ImVec2& p2, ImU32 color, float thickness)
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        float distance = sqrt(pow((p2.x - p1.x), 2.f) + pow((p2.y - p1.y), 2.f));
        float delta = distance * 0.45f;
        if (p2.x < p1.x) delta += 0.2f * (p1.x - p2.x);
        // float vert = (p2.x < p1.x - 20.f) ? 0.062f * distance * (p2.y - p1.y) * 0.005f : 0.f;
        float vert = 0.f;
        ImVec2 p22 = p2 - ImVec2(delta, vert);
        if (p2.x < p1.x - 50.f) delta *= -1.f;
        ImVec2 p11 = p1 + ImVec2(delta, vert);
        dl->AddBezierCubic(p1, p11, p22, p2, color, thickness);
    }

    inline bool smart_bezier_collider(const ImVec2& p, const ImVec2& p1, const ImVec2& p2, float radius)
    {
        float distance = sqrt(pow((p2.x - p1.x), 2.f) + pow((p2.y - p1.y), 2.f));
        float delta = distance * 0.45f;
        if (p2.x < p1.x) delta += 0.2f * (p1.x - p2.x);
        // float vert = (p2.x < p1.x - 20.f) ? 0.062f * distance * (p2.y - p1.y) * 0.005f : 0.f;
        float vert = 0.f;
        ImVec2 p22 = p2 - ImVec2(delta, vert);
        if (p2.x < p1.x - 50.f) delta *= -1.f;
        ImVec2 p11 = p1 + ImVec2(delta, vert);
        return ImProjectOnCubicBezier(p, p1, p11, p22, p2).Distance < radius;
    }

    // -----------------------------------------------------------------------------------------------------------------
    // HANDLER

    template<typename T, typename... Params>
    std::shared_ptr<T> ImNodeFlow::addNode(const ImVec2& pos, Params&&... args)
    {
        static_assert(std::is_base_of<BaseNode, T>::value, "Pushed type is not a subclass of BaseNode!");

        std::shared_ptr<T> n = std::make_shared<T>(std::forward<Params>(args)...);
        n->setPos(pos);
        n->setHandler(this);
        if (!n->getStyle())
            n->setStyle(NodeStyle::cyan());

        auto uid = reinterpret_cast<uintptr_t>(n.get());
        n->setUID(uid);
        m_nodes[uid] = n;
        return n;
    }

    template<typename T, typename... Params>
    std::shared_ptr<T> ImNodeFlow::placeNodeAt(const ImVec2& pos, Params&&... args)
    {
        return addNode<T>(screen2grid(pos), std::forward<Params>(args)...);
    }

    template<typename T, typename... Params>
    std::shared_ptr<T> ImNodeFlow::placeNode(Params&&... args)
    {
        return placeNodeAt<T>(ImGui::GetMousePos(), std::forward<Params>(args)...);
    }

    // -----------------------------------------------------------------------------------------------------------------
    // BASE NODE

    template<typename T>
    std::shared_ptr<InPin<T>> BaseNode::addIN(const std::string& name, T defReturn, std::function<bool(Pin*, Pin*)> filter, std::shared_ptr<PinStyle> style)
    {
        return addIN_uid(name, name, defReturn, std::move(filter), std::move(style));
    }

    template<typename T, typename U>
    std::shared_ptr<InPin<T>> BaseNode::addIN_uid(const U& uid, const std::string& name, T defReturn, std::function<bool(Pin*, Pin*)> filter, std::shared_ptr<PinStyle> style)
    {
        PinUID h = std::hash<U>{}(uid);
        auto p = std::make_shared<InPin<T>>(h, name, defReturn, std::move(filter), std::move(style), this, &m_inf);
        m_ins.emplace_back(p);
        return p;
    }

    template<typename U>
    void BaseNode::dropIN(const U& uid)
    {
        PinUID h = std::hash<U>{}(uid);
        for (auto it = m_ins.begin(); it != m_ins.end(); it++)
        {
            if (it->get()->getUid() == h)
            {
                m_ins.erase(it);
                return;
            }
        }
    }

    inline void BaseNode::dropIN(const char* uid)
    {
        dropIN<std::string>(uid);
    }

    template<typename T>
    const T& BaseNode::showIN(const std::string& name, T defReturn, std::function<bool(Pin*, Pin*)> filter, std::shared_ptr<PinStyle> style)
    {
        return showIN_uid(name, name, defReturn, std::move(filter), std::move(style));
    }

    template<typename T, typename U>
    const T& BaseNode::showIN_uid(const U& uid, const std::string& name, T defReturn, std::function<bool(Pin*, Pin*)> filter, std::shared_ptr<PinStyle> style)
    {
        PinUID h = std::hash<U>{}(uid);
        for (std::pair<int, std::shared_ptr<Pin>>& p : m_dynamicIns)
        {
            if (p.second->getUid() == h)
            {
                p.first = 1;
                return static_cast<InPin<T>*>(p.second.get())->val();
            }
        }

        m_dynamicIns.emplace_back(std::make_pair(1, std::make_shared<InPin<T>>(h, name, defReturn, std::move(filter), std::move(style), this, &m_inf)));
        return static_cast<InPin<T>*>(m_dynamicIns.back().second.get())->val();
    }

    template<typename T>
    std::shared_ptr<OutPin<T>> BaseNode::addOUT(const std::string& name, std::shared_ptr<PinStyle> style)
    {
        return addOUT_uid<T>(name, name, std::move(style));
    }

    template<typename T, typename U>
    std::shared_ptr<OutPin<T>> BaseNode::addOUT_uid(const U& uid, const std::string& name, std::shared_ptr<PinStyle> style)
    {
        PinUID h = std::hash<U>{}(uid);
        auto p = std::make_shared<OutPin<T>>(h, name, std::move(style), this, &m_inf);
        m_outs.emplace_back(p);
        return p;
    }

    template<typename U>
    void BaseNode::dropOUT(const U& uid)
    {
        PinUID h = std::hash<U>{}(uid);
        for (auto it = m_outs.begin(); it != m_outs.end(); it++)
        {
            if (it->get()->getUid() == h)
            {
                m_outs.erase(it);
                return;
            }
        }
    }

    inline void BaseNode::dropOUT(const char* uid)
    {
        dropOUT<std::string>(uid);
    }

    template<typename T>
    void BaseNode::showOUT(const std::string& name, std::function<T()> behaviour, std::shared_ptr<PinStyle> style)
    {
        showOUT_uid<T>(name, name, std::move(behaviour), std::move(style));
    }

    template<typename T, typename U>
    void BaseNode::showOUT_uid(const U& uid, const std::string& name, std::function<T()> behaviour, std::shared_ptr<PinStyle> style)
    {
        PinUID h = std::hash<U>{}(uid);
        for (std::pair<int, std::shared_ptr<Pin>>& p : m_dynamicOuts)
        {
            if (p.second->getUid() == h)
            {
                p.first = 2;
                return;
            }
        }

        m_dynamicOuts.emplace_back(std::make_pair(2, std::make_shared<OutPin<T>>(h, name, std::move(style), this, &m_inf)));
        static_cast<OutPin<T>*>(m_dynamicOuts.back().second.get())->behaviour(std::move(behaviour));
    }

    template<typename T, typename U>
    const T& BaseNode::getInVal(const U& uid)
    {
        PinUID h = std::hash<U>{}(uid);
        auto it = std::find_if(m_ins.begin(), m_ins.end(), [&h](std::shared_ptr<Pin>& p)
                            { return p->getUid() == h; });
        assert(it != m_ins.end() && "Pin UID not found!");
        return static_cast<InPin<T>*>(it->get())->val();
    }

    template<typename T>
    const T& BaseNode::getInVal(const char* uid)
    {
        return getInVal<T, std::string>(uid);
    }

    template<typename U>
    Pin* BaseNode::inPin(const U& uid)
    {
        PinUID h = std::hash<U>{}(uid);
        auto it = std::find_if(m_ins.begin(), m_ins.end(), [&h](std::shared_ptr<Pin>& p)
                            { return p->getUid() == h; });
        assert(it != m_ins.end() && "Pin UID not found!");
        return it->get();
    }

    inline Pin* BaseNode::inPin(const char* uid)
    {
        return inPin<std::string>(uid);
    }

    template<typename U>
    Pin* BaseNode::outPin(const U& uid)
    {
        PinUID h = std::hash<U>{}(uid);
        auto it = std::find_if(m_outs.begin(), m_outs.end(), [&h](std::shared_ptr<Pin>& p)
                            { return p->getUid() == h; });
        assert(it != m_outs.end() && "Pin UID not found!");
        return it->get();
    }

    inline Pin* BaseNode::outPin(const char* uid)
    {
        return outPin<std::string>(uid);
    }

    // -----------------------------------------------------------------------------------------------------------------
    // PIN

    inline void Pin::drawSocket()
    {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 tl = pinPoint() - ImVec2(m_style->socket_radius, m_style->socket_radius);
        ImVec2 br = pinPoint() + ImVec2(m_style->socket_radius, m_style->socket_radius);

        if (isConnected())
            draw_list->AddCircleFilled(pinPoint(), m_style->socket_connected_radius, m_style->color, m_style->socket_shape);
        else
        {
            if (ImGui::IsItemHovered() || ImGui::IsMouseHoveringRect(tl, br))
                draw_list->AddCircle(pinPoint(), m_style->socket_hovered_radius, m_style->color, m_style->socket_shape, m_style->socket_thickness);
            else
                draw_list->AddCircle(pinPoint(), m_style->socket_radius, m_style->color, m_style->socket_shape, m_style->socket_thickness);
        }

        if (ImGui::IsMouseHoveringRect(tl, br))
            (*m_inf)->hovering(this);
    }

    inline void Pin::drawDecoration()
    {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        if (ImGui::IsItemHovered())
            draw_list->AddRectFilled(m_pos - m_style->extra.padding, m_pos + m_size + m_style->extra.padding, m_style->extra.bg_hover_color, m_style->extra.bg_radius);
        else
            draw_list->AddRectFilled(m_pos - m_style->extra.padding, m_pos + m_size + m_style->extra.padding, m_style->extra.bg_color, m_style->extra.bg_radius);
        draw_list->AddRect(m_pos - m_style->extra.padding, m_pos + m_size + m_style->extra.padding, m_style->extra.border_color, m_style->extra.bg_radius, 0, m_style->extra.border_thickness);
    }

    inline void Pin::update()
    {
        // Custom rendering
        if (m_renderer)
        {
            ImGui::BeginGroup();
            m_renderer(this);
            ImGui::EndGroup();
            m_size = ImGui::GetItemRectSize();
            if (ImGui::IsItemHovered())
                (*m_inf)->hovering(this);
            return;
        }

        ImGui::SetCursorPos(m_pos);
        ImGui::Text("%s", m_name.c_str());
        m_size = ImGui::GetItemRectSize();

        drawDecoration();
        drawSocket();

        if (ImGui::IsItemHovered())
            (*m_inf)->hovering(this);
    }

    // -----------------------------------------------------------------------------------------------------------------
    // IN PIN

    template<class T>
    const T& InPin<T>::val()
    {
        if(!m_link)
            return m_emptyVal;

        return reinterpret_cast<OutPin<T>*>(m_link->left())->val();
    }

    template<class T>
    void InPin<T>::createLink(Pin *other)
    {
        if (other == this || other->getType() == PinType_Input)
            return;

        if (m_parent == other->getParent() && !m_allowSelfConnection)
            return;

        if (m_link && m_link->left() == other)
        {
            m_link.reset();
            return;
        }

        if (!m_filter(other, this)) // Check Filter
            return;

        m_link = std::make_shared<Link>(other, this, (*m_inf));
        other->setLink(m_link);
        (*m_inf)->addLink(m_link);
    }

    // -----------------------------------------------------------------------------------------------------------------
    // OUT PIN

    template<class T>
    const T &OutPin<T>::val()
    {
        std::string s = std::to_string(m_uid) + std::to_string(m_parent->getUID());
        if (std::find((*m_inf)->get_recursion_blacklist().begin(), (*m_inf)->get_recursion_blacklist().end(), s) == (*m_inf)->get_recursion_blacklist().end())
        {
            (*m_inf)->get_recursion_blacklist().emplace_back(s);
            m_val = m_behaviour();
        }

        return m_val;
    }

    template<class T>
    void OutPin<T>::createLink(ImFlow::Pin *other)
    {
        if (other == this || other->getType() == PinType_Output)
            return;

        other->createLink(this);
    }

    template<class T>
    void OutPin<T>::setLink(std::shared_ptr<Link>& link)
    {
        m_links.emplace_back(link);
    }

    template<class T>
    void OutPin<T>::deleteLink()
    {
        m_links.erase(std::remove_if(m_links.begin(), m_links.end(),
                                     [](const std::weak_ptr<Link>& l) { return l.expired(); }), m_links.end());
    }
}
