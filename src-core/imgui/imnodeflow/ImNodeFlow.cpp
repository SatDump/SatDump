#include "ImNodeFlow.h"

namespace ImFlow {
    // -----------------------------------------------------------------------------------------------------------------
    // LINK

    void Link::update() {
        ImVec2 start = m_left->pinPoint();
        ImVec2 end = m_right->pinPoint();
        float thickness = m_left->getStyle()->extra.link_thickness;
        bool mouseClickState = m_inf->getSingleUseClick();

        if (!ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            m_selected = false;

        if (smart_bezier_collider(ImGui::GetMousePos(), start, end, 2.5)) {
            m_hovered = true;
            thickness = m_left->getStyle()->extra.link_hovered_thickness;
            if (mouseClickState) {
                m_inf->consumeSingleUseClick();
                m_selected = true;
            }
        } else { m_hovered = false; }

        if (m_selected)
            smart_bezier(start, end, m_left->getStyle()->extra.outline_color,
                         thickness + m_left->getStyle()->extra.link_selected_outline_thickness);
        smart_bezier(start, end, m_left->getStyle()->color, thickness);

        if (m_selected && ImGui::IsKeyPressed(ImGuiKey_Delete, false))
            m_right->deleteLink();
    }

    Link::~Link() {
        m_left->deleteLink();
    }

    // -----------------------------------------------------------------------------------------------------------------
    // BASE NODE

    bool BaseNode::isHovered() {
        ImVec2 paddingTL = {m_style->padding.x, m_style->padding.y};
        ImVec2 paddingBR = {m_style->padding.z, m_style->padding.w};
        return ImGui::IsMouseHoveringRect(m_inf->grid2screen(m_pos - paddingTL),
                                          m_inf->grid2screen(m_pos + m_size + paddingBR));
    }

    void BaseNode::update() {
        ImDrawList *draw_list = ImGui::GetWindowDrawList();
        ImGui::PushID(this);
        bool mouseClickState = m_inf->getSingleUseClick();
        ImVec2 offset = m_inf->grid2screen({0.f, 0.f});
        ImVec2 paddingTL = {m_style->padding.x, m_style->padding.y};
        ImVec2 paddingBR = {m_style->padding.z, m_style->padding.w};

        draw_list->ChannelsSetCurrent(1); // Foreground
        ImGui::SetCursorScreenPos(offset + m_pos);

        ImGui::BeginGroup();

        // Header
        ImGui::BeginGroup();
        ImGui::TextColored(m_style->header_title_color, "%s", m_title.c_str());
        ImGui::Spacing();
        ImGui::EndGroup();
        float headerH = ImGui::GetItemRectSize().y;
        float titleW = ImGui::GetItemRectSize().x;

        // Inputs
        ImGui::BeginGroup();
        for (auto &p: m_ins) {
            p->setPos(ImGui::GetCursorPos());
            p->update();
        }
        for (auto &p: m_dynamicIns) {
            if (p.first == 1) {
                p.second->setPos(ImGui::GetCursorPos());
                p.second->update();
                p.first = 0;
            }
        }
        ImGui::EndGroup();
        ImGui::SameLine();

        // Content
        ImGui::BeginGroup();
        draw();
        ImGui::Dummy(ImVec2(0.f, 0.f));
        ImGui::EndGroup();
        ImGui::SameLine();

        // Outputs
        float maxW = 0.0f;
        for (auto &p: m_outs) {
            float w = p->calcWidth();
            if (w > maxW)
                maxW = w;
        }
        for (auto &p: m_dynamicOuts) {
            float w = p.second->calcWidth();
            if (w > maxW)
                maxW = w;
        }
        ImGui::BeginGroup();
        for (auto &p: m_outs) {
            // FIXME: This looks horrible
            if ((m_pos + ImVec2(titleW, 0) + m_inf->getGrid().scroll()).x <
                ImGui::GetCursorPos().x + ImGui::GetWindowPos().x + maxW)
                p->setPos(ImGui::GetCursorPos() + ImGui::GetWindowPos() + ImVec2(maxW - p->calcWidth(), 0.f));
            else
                p->setPos(ImVec2((m_pos + ImVec2(titleW - p->calcWidth(), 0) + m_inf->getGrid().scroll()).x,
                                 ImGui::GetCursorPos().y + ImGui::GetWindowPos().y));
            p->update();
        }
        for (auto &p: m_dynamicOuts) {
            // FIXME: This looks horrible
            if ((m_pos + ImVec2(titleW, 0) + m_inf->getGrid().scroll()).x <
                ImGui::GetCursorPos().x + ImGui::GetWindowPos().x + maxW)
                p.second->setPos(
                        ImGui::GetCursorPos() + ImGui::GetWindowPos() + ImVec2(maxW - p.second->calcWidth(), 0.f));
            else
                p.second->setPos(
                        ImVec2((m_pos + ImVec2(titleW - p.second->calcWidth(), 0) + m_inf->getGrid().scroll()).x,
                               ImGui::GetCursorPos().y + ImGui::GetWindowPos().y));
            p.second->update();
            p.first -= 1;
        }

        ImGui::EndGroup();

        ImGui::EndGroup();
        m_size = ImGui::GetItemRectSize();
        ImVec2 headerSize = ImVec2(m_size.x + paddingBR.x, headerH);

        // Background
        draw_list->ChannelsSetCurrent(0);
        draw_list->AddRectFilled(offset + m_pos - paddingTL, offset + m_pos + m_size + paddingBR, m_style->bg,
                                 m_style->radius);
        draw_list->AddRectFilled(offset + m_pos - paddingTL, offset + m_pos + headerSize, m_style->header_bg,
                                 m_style->radius, ImDrawFlags_RoundCornersTop);

        ImU32 col = m_style->border_color;
        float thickness = m_style->border_thickness;
        ImVec2 ptl = paddingTL;
        ImVec2 pbr = paddingBR;
        if (m_selected) {
            col = m_style->border_selected_color;
            thickness = m_style->border_selected_thickness;
        }
        if (thickness < 0.f) {
            ptl.x -= thickness / 2;
            ptl.y -= thickness / 2;
            pbr.x -= thickness / 2;
            pbr.y -= thickness / 2;
            thickness *= -1.f;
        }
        draw_list->AddRect(offset + m_pos - ptl, offset + m_pos + m_size + pbr, col, m_style->radius, 0, thickness);


        if (ImGui::IsWindowHovered() && !ImGui::IsKeyDown(ImGuiKey_LeftCtrl) &&
            ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !m_inf->on_selected_node())
            selected(false);

        if (isHovered()) {
            m_inf->hoveredNode(this);
            if (mouseClickState) {
                selected(true);
                m_inf->consumeSingleUseClick();
            }
        }

        if (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_Delete) && !ImGui::IsAnyItemActive() && isSelected())
            destroy();

        bool onHeader = ImGui::IsMouseHoveringRect(offset + m_pos - paddingTL, offset + m_pos + headerSize);
        if (onHeader && mouseClickState) {
            m_inf->consumeSingleUseClick();
            m_dragged = true;
            m_inf->draggingNode(true);
        }
        if (m_dragged || (m_selected && m_inf->isNodeDragged())) {
            float step = m_inf->getStyle().grid_size / m_inf->getStyle().grid_subdivisions;
            m_posTarget += ImGui::GetIO().MouseDelta;
            // "Slam" The position
            m_pos.x = round(m_posTarget.x / step) * step;
            m_pos.y = round(m_posTarget.y / step) * step;

            if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                m_dragged = false;
                m_inf->draggingNode(false);
                m_posTarget = m_pos;
            }
        }
        ImGui::PopID();

        // Deleting dead pins
        m_dynamicIns.erase(std::remove_if(m_dynamicIns.begin(), m_dynamicIns.end(),
                                          [](const std::pair<int, std::shared_ptr<Pin>> &p) { return p.first == 0; }),
                           m_dynamicIns.end());
        m_dynamicOuts.erase(std::remove_if(m_dynamicOuts.begin(), m_dynamicOuts.end(),
                                           [](const std::pair<int, std::shared_ptr<Pin>> &p) { return p.first == 0; }),
                            m_dynamicOuts.end());
    }

    // -----------------------------------------------------------------------------------------------------------------
    // HANDLER

    int ImNodeFlow::m_instances = 0;

    bool ImNodeFlow::on_selected_node() {
        return std::any_of(m_nodes.begin(), m_nodes.end(),
                           [](const auto &n) { return n.second->isSelected() && n.second->isHovered(); });
    }

    bool ImNodeFlow::on_free_space() {
        return std::all_of(m_nodes.begin(), m_nodes.end(),
                           [](const auto &n) { return !n.second->isHovered(); })
               && std::all_of(m_links.begin(), m_links.end(),
                              [](const auto &l) { return !l.lock()->isHovered(); });
    }

    ImVec2 ImNodeFlow::screen2grid(const ImVec2 &p) {
        if (ImGui::GetCurrentContext() == m_context.getRawContext())
            return p - m_context.scroll();
        else
            return p - m_context.origin() - m_context.scroll() * m_context.scale();
    }

    ImVec2 ImNodeFlow::grid2screen(const ImVec2 &p) {
        if (ImGui::GetCurrentContext() == m_context.getRawContext())
            return p + m_context.scroll();
        else
            return p + m_context.origin() + m_context.scroll() * m_context.scale();
    }

    void ImNodeFlow::addLink(std::shared_ptr<Link> &link) {
        m_links.push_back(link);
    }

    void ImNodeFlow::update() {
        // Updating looping stuff
        m_hovering = nullptr;
        m_hoveredNode = nullptr;
        m_draggingNode = m_draggingNodeNext;
        m_singleUseClick = ImGui::IsMouseClicked(ImGuiMouseButton_Left);

        // Create child canvas
        m_context.begin();
        ImGui::GetIO().IniFilename = nullptr;

        ImDrawList *draw_list = ImGui::GetWindowDrawList();

        // Display grid
        ImVec2 gridSize = ImGui::GetWindowSize();
        float subGridStep = m_style.grid_size / m_style.grid_subdivisions;
        for (float x = fmodf(m_context.scroll().x, m_style.grid_size); x < gridSize.x; x += m_style.grid_size)
            draw_list->AddLine(ImVec2(x, 0.0f), ImVec2(x, gridSize.y), m_style.colors.grid);
        for (float y = fmodf(m_context.scroll().y, m_style.grid_size); y < gridSize.y; y += m_style.grid_size)
            draw_list->AddLine(ImVec2(0.0f, y), ImVec2(gridSize.x, y), m_style.colors.grid);
        if (m_context.scale() > 0.7f) {
            for (float x = fmodf(m_context.scroll().x, subGridStep); x < gridSize.x; x += subGridStep)
                draw_list->AddLine(ImVec2(x, 0.0f), ImVec2(x, gridSize.y), m_style.colors.subGrid);
            for (float y = fmodf(m_context.scroll().y, subGridStep); y < gridSize.y; y += subGridStep)
                draw_list->AddLine(ImVec2(0.0f, y), ImVec2(gridSize.x, y), m_style.colors.subGrid);
        }

        // Update and draw nodes
        // TODO: I don't like this
        draw_list->ChannelsSplit(2);
        for (auto &node: m_nodes) { node.second->update(); }
        // Remove "toDelete" nodes
        for (auto iter = m_nodes.begin(); iter != m_nodes.end();) {
            if (iter->second->toDestroy())
                iter = m_nodes.erase(iter);
            else
                ++iter;
        }
        draw_list->ChannelsMerge();
        for (auto &node: m_nodes) { node.second->updatePublicStatus(); }

        // Update and draw links
        for (auto &l: m_links) { if (!l.expired()) l.lock()->update(); }

        // Links drop-off
        if (m_dragOut && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            if (!m_hovering) {
                if (on_free_space() && m_droppedLinkPopUp) {
                    if (m_droppedLinkPupUpComboKey == ImGuiKey_None || ImGui::IsKeyDown(m_droppedLinkPupUpComboKey)) {
                        m_droppedLinkLeft = m_dragOut;
                        ImGui::OpenPopup("DroppedLinkPopUp");
                    }
                }
            } else
                m_dragOut->createLink(m_hovering);
        }

        // Links drag-out
        if (!m_draggingNode && m_hovering && !m_dragOut && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            m_dragOut = m_hovering;
        if (m_dragOut) {
            if (m_dragOut->getType() == PinType_Output)
                smart_bezier(m_dragOut->pinPoint(), ImGui::GetMousePos(), m_dragOut->getStyle()->color,
                             m_dragOut->getStyle()->extra.link_dragged_thickness);
            else
                smart_bezier(ImGui::GetMousePos(), m_dragOut->pinPoint(), m_dragOut->getStyle()->color,
                             m_dragOut->getStyle()->extra.link_dragged_thickness);

            if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
                m_dragOut = nullptr;
        }

        // Right-click PopUp
        if (m_rightClickPopUp && ImGui::IsMouseClicked(ImGuiMouseButton_Right) && ImGui::IsWindowHovered()) {
            m_hoveredNodeAux = m_hoveredNode;
            ImGui::OpenPopup("RightClickPopUp");
        }
        if (ImGui::BeginPopup("RightClickPopUp")) {
            m_rightClickPopUp(m_hoveredNodeAux);
            ImGui::EndPopup();
        }

        // Dropped Link PopUp
        if (ImGui::BeginPopup("DroppedLinkPopUp")) {
            m_droppedLinkPopUp(m_droppedLinkLeft);
            ImGui::EndPopup();
        }

        // Removing dead Links
        m_links.erase(std::remove_if(m_links.begin(), m_links.end(),
                                     [](const std::weak_ptr<Link> &l) { return l.expired(); }), m_links.end());

        // Clearing recursion blacklist
        m_pinRecursionBlacklist.clear();

        m_context.end();
    }
}
