#include "common/widgets/CircularProgressBar.h"
#include "core/style.h"
#include "dsp/block.h"
#include "dsp/block_helpers.h"
#include "flowgraph.h"
#include "imgui/imgui.h"
#include "imgui/imnodes/imnodes.h"
#include "utils/string.h"
#include <mutex>

namespace satdump
{
    namespace ndsp
    {
        namespace flowgraph
        {
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
                    if (ImGui::TreeNodeEx(c.first.c_str(), ImGuiTreeNodeFlags_Leaf))
                        ImGui::TreePop();
                    if (ImGui::IsItemClicked())
                        addNode(c.first, c.second.func(this));
                }
            }

            void Flowgraph::renderAddMenuList(std::string search)
            {
                std::lock_guard<std::mutex> lg(flow_mtx);

                // Extract categories
                std::vector<NodeInternalReg> regs;
                std::vector<std::vector<std::string>> categs;
                for (auto &opt : node_internal_registry)
                {
                    if (search.size() && !isStringPresent(opt.second.menuname, search))
                        continue;

                    regs.push_back(opt.second);
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
                            cCats->sub.emplace(cat, regs[i]);
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
            }

            void Flowgraph::render()
            {
                std::lock_guard<std::mutex> lg(flow_mtx);

                ImNodes::PushStyleVar(ImNodesStyleVar_PinCircleRadius, 6);
                ImNodes::PushStyleVar(ImNodesStyleVar_LinkThickness, 4);
                ImNodes::PushAttributeFlag(ImNodesAttributeFlags_EnableLinkDetachWithDragClick);

                ImNodes::BeginNodeEditor();
                ImNodes::MiniMap();

                for (auto &n : nodes)
                {
                    if (n->disabled)
                        style::beginDisabled();

                    // Make it red on error
                    bool is_error = n->internal->is_errored();
                    if (is_error)
                        ImNodes::PushColorStyle(ImNodesCol_NodeBackground, ImColor(255, 0, 0));

                    ImNodes::BeginNode(n->id);
                    ImNodes::BeginNodeTitleBar();
                    ImGui::Text("%s", n->title.c_str());
                    ImNodes::EndNodeTitleBar();

                    // Update if needed
                    if (n->internal->render())
                        n->updateIO();

                    int in_pos = 0;
                    for (auto &io : n->node_io)
                    {
                        ImNodes::PushColorStyle(ImNodesCol_Pin, getColorFromDSPType(io.type));
                        if (io.is_out)
                        {
                            ImNodes::BeginOutputAttribute(io.id);
                            const float node_width = ImNodes::GetNodeDimensions(n->id).x;
                            const float label_width = ImGui::CalcTextSize(io.name.c_str()).x;
                            ImGui::Indent(node_width - label_width - 20 * ui_scale);
                            ImGui::Text("%s", io.name.c_str());
                            ImNodes::EndOutputAttribute();
                        }
                        else
                        {
                            ImNodes::BeginInputAttribute(io.id);
                            ImGui::Text("%s", io.name.c_str());

                            if (is_running && debug_mode)
                            {
                                if (in_pos < n->internal->blk->get_inputs().size())
                                {
                                    auto f = n->internal->blk->get_inputs()[in_pos];

                                    if (f.fifo)
                                    {
                                        float v = (float)f.fifo->size_approx() / (float)f.fifo->max_capacity();
                                        ImGui::SameLine();
                                        if (v > 1)
                                            v = 1;
                                        if (v < 0)
                                            v = 0;
                                        CircularProgressBar(f.name.c_str(), v, {20, 20}, style::theme.green);
                                    }
                                }
                            }

                            ImNodes::EndInputAttribute();
                            in_pos++;
                        }
                        ImNodes::PopColorStyle();
                    }

                    ImNodes::EndNode();

                    if (is_error)
                        ImNodes::PopColorStyle();

                    if (!n->pos_was_set)
                    {
                        ImNodes::SetNodeGridSpacePos(n->id, {n->pos_x, n->pos_y});
                        n->pos_was_set = true;
                    }

                    auto pos = ImNodes::GetNodeGridSpacePos(n->id);
                    n->pos_x = pos.x;
                    n->pos_y = pos.y;

                    if (n->disabled)
                        style::endDisabled();
                }

                for (auto &l : links)
                {
                    bool disabled = false;

                    BlockIOType type;
                    for (auto &n : nodes)
                        for (auto &io : n->node_io)
                        {
                            if (io.id == l.start)
                            {
                                type = io.type;
                                if (n->disabled)
                                    disabled = true;
                            }
                            else if (io.id == l.end)
                            {
                                if (n->disabled)
                                    disabled = true;
                            }
                        }

                    ImNodes::PushColorStyle(ImNodesCol_Link, disabled ? (ImColor)ImGui::GetColorU32(ImGuiCol_TextDisabled) : getColorFromDSPType(type));
                    ImNodes::Link(l.id, l.start, l.end);
                    ImNodes::PopColorStyle();
                }

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

                    if (!ImGui::IsAnyItemActive() && ImGui::IsKeyPressed(ImGuiKey_D))
                    {
                        int node_s = ImNodes::NumSelectedNodes();

                        if (node_s > 0)
                        {
                            std::vector<int> nodes_ids(node_s);
                            ImNodes::GetSelectedNodes(nodes_ids.data());

                            for (auto &id : nodes_ids)
                            {
                                auto iter = std::find_if(nodes.begin(), nodes.end(), [id](const std::shared_ptr<Node> &node) -> bool { return node->id == id; });
                                iter->get()->disabled = !iter->get()->disabled;
                            }
                        }
                    }
                    ///////////////////// Node Key Handlers end

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

                if (!is_running)
                {
                    int id = 0;
                    if (ImNodes::IsNodeHovered(&id) && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                    {
                        for (auto &n : nodes)
                            if (n->id == id)
                                n->show_vars_win = true;
                    }
                }
            }

            void Flowgraph::renderWindows()
            {
                std::lock_guard<std::mutex> lg(flow_mtx);

                for (auto &n : nodes)
                {
                    if (n->show_vars_win)
                    {
                        std::string name = n->internal->blk->d_id + "##nodevarspopup";
                        ImGui::Begin(name.c_str(), &n->show_vars_win, ImGuiWindowFlags_AlwaysAutoResize);
                        for (auto &v : n->vars)
                        {
                            ImGui::SeparatorText(v.first.c_str());
                            ImGui::InputText(std::string("##" + v.first).c_str(), &v.second);
                            ImGui::SameLine();

                            if (ImGui::Button(("Delete##" + name + v.first).c_str()))
                            {
                                n->vars.erase(v.first);
                                break;
                            }
                        }

                        ImGui::SeparatorText("Add others");

                        int nl = 1;
                        for (auto &v : n->internal->blk->get_cfg().items())
                        {
                            bool found = false;
                            for (auto &v2 : n->vars)
                                if (v2.first == v.key())
                                    found = true;
                            if (found)
                                continue;

                            std::string bname = v.key() + "##nodeaddbtn";
                            if (ImGui::Button(bname.c_str()))
                            {
                                n->vars.emplace(v.key(), "");
                            }

                            if (nl && !(nl % 3 == 0))
                                ImGui::SameLine();
                            nl++;
                        }

                        ImGui::End();
                    }
                }
            }
        } // namespace flowgraph
    } // namespace ndsp
} // namespace satdump