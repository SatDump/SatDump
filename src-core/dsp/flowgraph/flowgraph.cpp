#include "flowgraph.h"
#include "core/exception.h"
#include <chrono>
#include <limits>

#include "imgui/imnodes/imnodes.h"
#include "imgui/imnodes/imnodes_internal.h"
#include "logger.h"

#include "utils/string.h"

#include "dsp/path/splitter.h"

#include <thread>

namespace satdump
{
    namespace ndsp
    {
        Flowgraph::Flowgraph() {}

        Flowgraph::~Flowgraph() {}

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

        void Flowgraph::renderAddMenu(std::pair<const std::string, NodeInternalReg> &opt, std::vector<std::string> cats, int pos)
        {
            if (pos == (cats.size() - 1))
            {
                if (ImGui::MenuItem(cats[pos].c_str()))
                {
                    auto mpos = ImGui::GetMousePos();
                    auto ptr = addNode(opt.first, opt.second.func(this));
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

        void Flowgraph::render()
        {
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

                if (n->internal->render())
                    n->updateIO(); // TODOREWORK!

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

            int start_att, end_att;
            if (ImNodes::IsLinkCreated(&start_att, &end_att))
            {
                links.push_back({getNewLinkID(), start_att, end_att});
                logger->trace("LINK CREATE %d %d", start_att, end_att);
            }

            int link_id;
            if (ImNodes::IsLinkDestroyed(&link_id))
            {
                auto iter = std::find_if(links.begin(), links.end(), [link_id](const Link &link) -> bool { return link.id == link_id; });
                logger->trace("LINK DELETE %d %d", iter->start, iter->end);
                links.erase(iter);
            }

            if (ImGui::IsKeyPressed(ImGuiKey_Delete))
            {
                int node_s = ImNodes::NumSelectedNodes();

                if (node_s > 0)
                {
                    std::vector<int> nodes_ids(node_s);
                    ImNodes::GetSelectedNodes(nodes_ids.data());

                    for (auto &id : nodes_ids)
                    {
                        auto iter = std::find_if(nodes.begin(), nodes.end(), [id](const std::shared_ptr<Node> &node) -> bool { return node->id == id; });
                        logger->trace("NODE DELETE %d", id);
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

        void Flowgraph::run()
        {
            is_running = true;

            try
            {
                // Holds stuff such as splitters when one output goes to more than
                // one input
                std::vector<std::shared_ptr<ndsp::Block>> additional_blocks;

                // Iterate through all nodes
                for (auto &n : nodes)
                {
                    //                    n->internal->applyP(); // TODOREWORK?
                    auto &blk = n->internal->blk;

                    // Iterate through outputs
                    for (int o = 0; o < blk->get_outputs().size(); o++)
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

                        // Iterate through links, to asign outputs to applicable inputs
                        struct InputH
                        {
                            std::shared_ptr<ndsp::Block> blk;
                            int idx;
                        };
                        std::vector<InputH> inputs_to_feed;
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
                                                // n2->internal->blk->inputs[b2] = blk->outputs[o];
                                                inputs_to_feed.push_back({n2->internal->blk, b2}); //  &n2->internal->blk->get_output(b2, 16 /*TODOREWORK*/));
                                                logger->trace("Assigned to : " + n2->internal->blk->d_id);
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
                                                // n2->internal->blk->inputs[b2] = blk->outputs[o];
                                                inputs_to_feed.push_back({n2->internal->blk, b2}); // inputs_to_feed.push_back(&n2->internal->blk->get_output(b2, 16 /*TODOREWORK*/));
                                                logger->trace("Assigned to : " + n2->internal->blk->d_id);
                                            }

                                            b2++;
                                        }
                                    }
                                }
                            }
                        }

                        if (inputs_to_feed.size() == 1)
                        {
                            inputs_to_feed[0].blk->link(blk.get(), o, inputs_to_feed[0].idx, 16 /*TODOREWORK*/); // = blk->get_output(o, 16 /*TODOREWORK*/);
                        }
                        else if (inputs_to_feed.size() > 1)
                        {
                            logger->error("More than one to connect! Adding splitter");

                            std::shared_ptr<ndsp::Block> ptr;
                            if (blk->get_output(o, 0).type == ndsp::BlockIOType::DSP_SAMPLE_TYPE_CF32)
                                ptr = std::make_shared<ndsp::SplitterBlock<complex_t>>();
                            else if (blk->get_output(o, 0).type == ndsp::BlockIOType::DSP_SAMPLE_TYPE_F32)
                                ptr = std::make_shared<ndsp::SplitterBlock<float>>();
                            else
                                throw satdump_exception("Unsupported splitter block IO type");

                            nlohmann::json p;
                            p["noutputs"] = inputs_to_feed.size();
                            ptr->set_cfg(p);

                            ptr->link(blk.get(), o, 0, 16 /*TODOREWORK*/); // ptr->inputs[0] = blk->outputs[o];
                            for (int v = 0; v < inputs_to_feed.size(); v++)
                                inputs_to_feed[v].blk->link(ptr.get(), v, inputs_to_feed[0].idx, 16 /*TODOREWORK*/); //(*inputs_to_feed[v]) = ptr->outputs[v];

                            additional_blocks.push_back(ptr);
                        }
                        else if (inputs_to_feed.size() == 0)
                        {
                            throw satdump_exception("Block has unconnected output!");
                        }
                    }
                }

                // Start them all
                for (auto &n : nodes)
                    n->internal->blk->start();
                for (auto &b : additional_blocks)
                    b->start();
                for (auto &n : nodes)
                    n->internal->up_state();

                // And then wait for them to exit
                for (auto &n : nodes)
                    n->internal->blk->stop();
                for (auto &b : additional_blocks)
                    b->stop();

                std::this_thread::sleep_for(std::chrono::seconds(2));

                for (auto &n : nodes)
                    n->internal->up_state();
            }
            catch (std::exception &e)
            {
                logger->error("Error running flowgraph : %s", e.what());
            }

            is_running = false;
        }

        void Flowgraph::stop()
        {
            try
            {
                // Iterate through all nodes
                for (auto &n : nodes)
                {
                    // Stop only those that are sources
                    if (n->internal->blk->get_inputs().size() == 0)
                    {
                        logger->trace("Stopping source " + n->internal->blk->d_id);
                        n->internal->blk->stop(true);
                        logger->trace("Stopped source " + n->internal->blk->d_id);
                    }
                }
            }
            catch (std::exception &e)
            {
                logger->error("Error running flowgraph : %s", e.what());
            }
        }
    } // namespace ndsp
} // namespace satdump
