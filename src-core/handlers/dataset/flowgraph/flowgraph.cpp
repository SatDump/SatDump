#include "flowgraph.h"
#include "core/exception.h"
#include <limits>

#include "core/style.h"
#include "imgui/imnodes/imnodes.h"
#include "imgui/imnodes/imnodes_internal.h"
#include "logger.h"
#include "utils/string.h"

namespace satdump
{
    Flowgraph::Flowgraph() {}

    Flowgraph::~Flowgraph() {}

    void Flowgraph::renderAddMenu(std::pair<const std::string, NodeInternalReg> &opt, std::vector<std::string> cats, int pos)
    {
        if (pos == (cats.size() - 1))
        {
            if (ImGui::MenuItem(cats[pos].c_str()))
            {
                auto mpos = ImGui::GetMousePos();
                auto ptr = addNode(opt.first, opt.second.inst());
                ptr->pos_was_set = true;
                ImNodes::SetNodeScreenSpacePos(ptr->id, mpos);
            }
        }
        else
        {
            if (ImGui::BeginMenu(cats[pos].c_str()))
            {
                renderAddMenu(opt, cats, pos + 1);
                ImGui::EndMenu();
            }
        }
    }

    void Flowgraph::renderCatT(CatT &cats, bool searching)
    {
        for (auto &c : cats.cats)
        {
            if (searching)
                ImGui::SetNextItemOpen(true);

            if (ImGui::TreeNode(c.first.c_str()))
            {
                renderCatT(c.second, searching);
                ImGui::TreePop();
            }
        }

        for (auto &c : cats.sub)
        {
            auto split = splitString(c.second.menuname, '/');
            std::string name = split.size() ? split[split.size() - 1] : c.second.menuname;

            if (ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_Leaf))
                ImGui::TreePop();
            if (ImGui::IsItemClicked())
                addNode(c.first, c.second.inst());
        }
    }

    void Flowgraph::renderAddMenuList(std::string search)
    {
        std::lock_guard<std::mutex> lg(flow_mtx);

        bool is_run = is_running;
        if (is_run)
            style::beginDisabled();

        // Extract categories
        std::vector<std::pair<std::string, NodeInternalReg>> regs;
        std::vector<std::vector<std::string>> categs;
        for (auto &opt : node_internal_registry)
        {
            if (search.size() && !isStringPresent(opt.second.menuname, search))
                continue;

            regs.push_back({opt.first, opt.second});
            categs.push_back(splitString(opt.second.menuname, '/'));
        }

        CatT cats;

        // Organize in a recursive fashion
        for (int i = 0; i < regs.size(); i++)
        {
            CatT *cCats = &cats;
            for (int c = 0; c < categs[i].size(); c++)
            {
                std::string cat = categs[i][c];

                if (c == categs[i].size() - 1)
                {
                    cCats->sub.emplace(regs[i].first, regs[i].second);
                }
                else
                {
                    if (!cCats->cats.count(cat))
                        cCats->cats.emplace(cat, CatT());
                    cCats = &cCats->cats[cat];
                }
            }
        }

        // Render
        renderCatT(cats, search.size());

        if (is_run)
            style::endDisabled();
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
        std::lock_guard<std::mutex> lg(flow_mtx);

        ImNodes::PushStyleVar(ImNodesStyleVar_PinCircleRadius, 6);
        ImNodes::PushStyleVar(ImNodesStyleVar_LinkThickness, 4);
        ImNodes::PushAttributeFlag(ImNodesAttributeFlags_EnableLinkDetachWithDragClick);

        ImNodes::BeginNodeEditor();
        ImNodes::MiniMap();

        //        ImNodes::PushColorStyle(ImNodesCol_TitleBar, 0xc01c28FF);

        for (auto &n : nodes)
        {
            ImNodes::BeginNode(n->id);
            ImNodes::BeginNodeTitleBar();
            ImGui::Text("%s", n->title.c_str());
            ImNodes::EndNodeTitleBar();

            n->internal->render();

            for (auto &io : n->node_io)
            {
                if (io.is_out)
                {
                    ImNodes::BeginOutputAttribute(io.id);
                    //                    const float node_width = 200.0 * ui_scale;
                    //                    const float label_width = ImGui::CalcTextSize(io.name.c_str()).x;
                    //                    ImGui::Indent(node_width - label_width);
                    ImGui::Text("%s", io.name.c_str());
                    ImNodes::EndOutputAttribute();
                }
                else
                {
                    ImNodes::BeginInputAttribute(io.id);
                    ImGui::Text("%s", io.name.c_str());
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

        //        ImNodes::PopColorStyle();

        for (auto &l : links)
            ImNodes::Link(l.id, l.start, l.end);

        ImNodes::EndNodeEditor();

        ImNodes::PopAttributeFlag();
        ImNodes::PopStyleVar();
        ImNodes::PopStyleVar();

        // Lock down edition when running!
        if (!is_running)
        {
            int start_att, end_att;
            if (ImNodes::IsLinkCreated(&start_att, &end_att))
                links.push_back({getNewLinkID(), start_att, end_att});

            int link_id;
            if (ImNodes::IsLinkDestroyed(&link_id))
            {
                auto iter = std::find_if(links.begin(), links.end(), [link_id](const Link &link) -> bool { return link.id == link_id; });
                links.erase(iter);
            }

            ///////////////////// Node Key Handlers
            if (!ImGui::IsAnyItemActive() && ImGui::IsKeyPressed(ImGuiKey_Delete))
            {
                int node_s = ImNodes::NumSelectedNodes();

                if (node_s > 0)
                {
                    std::vector<int> nodes_ids(node_s);
                    ImNodes::GetSelectedNodes(nodes_ids.data());

                    for (auto &id : nodes_ids)
                    {
                        auto iter = std::find_if(nodes.begin(), nodes.end(), [id](const std::shared_ptr<Node> &node) -> bool { return node->id == id; });

                        // ImNodes still holds delete selected nodes until a new one is selected. Avoid a crash
                        if (iter == nodes.end())
                            continue;

                        for (auto &linkid : iter->get()->node_io)
                        {
                            auto liter = std::find_if(links.begin(), links.end(), [linkid](const Link &link) -> bool { return link.start == linkid.id || link.end == linkid.id; });
                            if (liter != links.end())
                                links.erase(liter);
                        }

                        nodes.erase(iter);
                    }
                }
            }

            if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
                ImGui::OpenPopup("##popuprightclickflowgraph");
            if (ImGui::BeginPopup("##popuprightclickflowgraph"))
            {
                if (ImGui::BeginMenu("Add Node"))
                {
                    for (auto &opt : node_internal_registry)
                    {
                        std::vector<std::string> cats = splitString(opt.second.menuname, '/');
                        renderAddMenu(opt, cats, 0);
                    }
                    ImGui::EndMenu();
                }

                ImGui::EndPopup();
            }
        }
    }

    void Flowgraph::run()
    {
        is_running = true;

        for (auto &n : nodes)
            n->internal->reset();

        try
        {
            bool should_run_again = true;
            int step_number = 0;
            while (should_run_again)
            {
                should_run_again = false;

                // Iterate through all nodes
                for (auto &n : nodes)
                {
                    // Check if this one can run
                    auto &i = n->internal;
                    if (i->can_run() && !i->has_run)
                    {
                        // Run it.
                        i->process();
                        logger->debug("Step %d Ran : %s", step_number, i->title.c_str());

                        // Iterate through outputs
                        for (int o = 0; o < i->outputs.size(); o++)
                        {
                            // Get output ID
                            int o_id = -1;
                            for (int c = 0, oc = 0; c < n->node_io.size(); c++)
                            {
                                if (n->node_io[c].is_out)
                                {
                                    if (oc == o)
                                        o_id = n->node_io[c].id;
                                    oc++;
                                }
                            }

                            logger->trace("Output ID for %d is %d", o, o_id);

                            // Iterate through links, to asign outputs to applicable inputs
                            for (auto &l : links)
                            {
                                if (l.start == o_id)
                                {
                                    // Iterate through nodes to find valid inputs
                                    for (auto &n2 : nodes)
                                    {
                                        for (int b = 0, b2 = 0; b < n2->node_io.size(); b++)
                                        {
                                            if (!n2->node_io[b].is_out)
                                            {
                                                if (n2->node_io[b].id == l.end)
                                                {
                                                    if (n2->internal->inputs[b2].type == i->outputs[o].type)
                                                    {
                                                        n2->internal->inputs[b2].ptr = i->outputs[o].ptr;
                                                        logger->trace("Assigned to : " + n2->internal->title);
                                                    }
                                                    else
                                                    {
                                                        throw satdump_exception("Incompatible types! " + n2->internal->inputs[b2].type + " and " + i->outputs[o].type);
                                                    }
                                                }

                                                b2++;
                                            }
                                        }
                                    }
                                }

                                if (l.end == o_id)
                                {
                                    // Iterate through nodes to find valid inputs
                                    for (auto &n2 : nodes)
                                    {
                                        for (int b = 0, b2 = 0; b < n2->node_io.size(); b++)
                                        {
                                            if (!n2->node_io[b].is_out)
                                            {
                                                if (n2->node_io[b].id == l.start)
                                                {
                                                    n2->internal->inputs[b2] = i->outputs[o];
                                                    logger->trace("Assigned to : " + n2->internal->title);
                                                }

                                                b2++;
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        should_run_again = true;
                    }
                }

                step_number++;
            }

            logger->info("Flowgraph took %d steps to run!", step_number);
        }
        catch (std::exception &e)
        {
            logger->error("Error running flowgraph : %s", e.what());
        }

        for (auto &n : nodes)
            n->internal->reset();

        is_running = false;
    }
} // namespace satdump