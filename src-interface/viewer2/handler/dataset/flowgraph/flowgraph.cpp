#include "flowgraph.h"
#include <limits>
#include "core/exception.h"

#include "imgui/imnodes/imnodes.h"
#include "logger.h"

namespace satdump
{
    Flowgraph::Flowgraph()
    {
    }

    Flowgraph::~Flowgraph()
    {
    }

    int Flowgraph::getNewNodeID()
    {
        for (int i = 0; i < std::numeric_limits<int>::max(); i++)
        {
            bool already_contained = false;
            for (auto &n : nodes)
                if (n->id == i)
                    already_contained = true;
            if (already_contained)
                continue;
            return i;
        }

        throw satdump_exception("No valid ID found for new node ID!");
    }

    int Flowgraph::getNewNodeIOID(std::vector<Node::InOut> *ptr)
    {
        for (int i = 0; i < std::numeric_limits<int>::max(); i++)
        {
            bool already_contained = false;
            for (auto &n : nodes)
                for (auto &io : n->node_io)
                    if (io.id == i)
                        already_contained = true;
            if (ptr != nullptr)
                for (auto &io : *ptr)
                    if (io.id == i)
                        already_contained = true;
            if (already_contained)
                continue;
            return i;
        }

        throw satdump_exception("No valid ID found for new node IO ID!");
    }

    int Flowgraph::getNewLinkID()
    {
        for (int i = 0; i < std::numeric_limits<int>::max(); i++)
        {
            bool already_contained = false;
            for (auto &l : links)
                if (l.id == i)
                    already_contained = true;
            if (already_contained)
                continue;
            return i;
        }

        throw satdump_exception("No valid ID found for new link ID!");
    }

    void Flowgraph::render()
    {
        ImNodes::PushAttributeFlag(ImNodesAttributeFlags_EnableLinkDetachWithDragClick);

        ImNodes::BeginNodeEditor();

        for (auto &n : nodes)
        {
            ImNodes::BeginNode(n->id);
            ImNodes::BeginNodeTitleBar();
            ImGui::Text(n->title.c_str());
            ImNodes::EndNodeTitleBar();

            n->internal->render();

            for (auto &io : n->node_io)
            {
                if (io.is_out)
                {
                    ImNodes::BeginOutputAttribute(io.id);
                    ImGui::Text(io.name.c_str());
                    ImNodes::EndOutputAttribute();
                }
                else
                {
                    ImNodes::BeginInputAttribute(io.id);
                    ImGui::Text(io.name.c_str());
                    ImNodes::EndInputAttribute();
                }
            }

            ImNodes::EndNode();

            if (!n->pos_was_set)
            {
                ImNodes::SetNodeGridSpacePos(n->id, {n->pos_x, n->pos_y});
                n->pos_was_set = true;
            }

            auto pos = ImNodes::GetNodeGridSpacePos(n->id);
            n->pos_x = pos.x;
            n->pos_y = pos.y;
        }

        for (auto &l : links)
            ImNodes::Link(l.id, l.start, l.end);

        ImNodes::EndNodeEditor();

        int start_att, end_att;
        if (ImNodes::IsLinkCreated(&start_att, &end_att))
        {
            links.push_back({getNewLinkID(), start_att, end_att});
            logger->trace("LINK CREATE %d %d", start_att, end_att);
        }

        int link_id;
        if (ImNodes::IsLinkDestroyed(&link_id))
        {
            auto iter = std::find_if(
                links.begin(), links.end(), [link_id](const Link &link) -> bool
                { return link.id == link_id; });
            logger->trace("LINK DELETE %d %d", iter->start, iter->end);
            links.erase(iter);
        }
    }
}