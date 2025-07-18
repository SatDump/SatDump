#include "handler.h"

#include "core/style.h"
#include "imgui/imgui.h"
#include "logger.h"
#include <utility>

namespace satdump
{
    namespace handlers
    {
        inline ImGuiTreeNodeFlags nodeFlags(std::shared_ptr<Handler> &h, bool sec)
        {
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
            //  if (!h->hasSubhandlers())
            flags |= ImGuiTreeNodeFlags_Leaf;
            if (sec)
                flags |= ImGuiTreeNodeFlags_Selected;
            return flags;
        }

        bool Handler::isParent(const std::shared_ptr<Handler> &dragged, const std::shared_ptr<Handler> &potential_child)
        {
            for (auto &s : dragged->subhandlers)
            {
                if (s == potential_child)
                    return true;
                if (isParent(s, potential_child))
                    return true;
            }
            return false;
        }

        void Handler::drawTreeMenu(std::shared_ptr<Handler> &h)
        {
            struct DragDropWip
            {
                std::shared_ptr<Handler> drag;
                Handler *hdel;
            };

            tree_local.start();
            for (int i = 0; i < subhandlers.size(); i++)
            {
                auto &handler = subhandlers[i];
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);

                bool tree_extended = ImGui::TreeNodeEx(handler->getTreeID().c_str(), nodeFlags(handler, h == handler));
                tree_local.node(handler->handler_tree_icon);

                if (tree_extended)
                {
                    if (ImGui::IsItemClicked())
                        h = handler;

                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("%s", handler->getName().c_str());

                    if (handler_can_subhandlers_be_dragged && handler->handler_can_be_dragged && ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
                    {
                        DragDropWip d({handler, this});
                        ImGui::SetDragDropPayload("EXPLORER_HANDLER", &d, sizeof(DragDropWip));

                        ImGui::Text("%s", handler->getName().c_str());
                        ImGui::EndDragDropSource();
                    }

                    if (handler->handler_can_be_dragged_to && ImGui::BeginDragDropTarget())
                    {
                        if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("EXPLORER_HANDLER"))
                        {
                            IM_ASSERT(payload->DataSize == sizeof(DragDropWip));
                            const DragDropWip *payload_n = (const DragDropWip *)payload->Data;
                            logger->info("Handler " + handler->getName() + " got drag of " + payload_n->drag->getName());
                            if (!isParent(payload_n->drag, handler))
                            {
                                handler->addSubHandler(payload_n->drag);
                                payload_n->hdel->delSubHandler(payload_n->drag);
                            }
                            else
                            {
                                logger->error("Can't drag parent onto child handler!");
                            }
                        }
                        ImGui::EndDragDropTarget();
                    }

                    if (handler_can_be_reorg)
                    {
                        float xpos = ImGui::GetWindowWidth();
                        float ypos = ImGui::GetCursorPosY();

                        ImVec4 c = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
                        c.w = 1;

                        if (i > 0)
                        {
                            ImGui::GetWindowDrawList()->AddRectFilled({xpos - 32 * ui_scale, ypos + 17 * ui_scale}, //
                                                                      {xpos - 10 * ui_scale, ypos + 32 * ui_scale}, //
                                                                      (ImU32)((ImColor)c));

                            ImGui::SameLine();
                            ImGui::SetCursorPosX(xpos - 35 * ui_scale);
                            ImGui::Text(u8"\ueaf4");
                            if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
                                std::swap(subhandlers[i], subhandlers[i - 1]);
                        }
                        if (i != (int)subhandlers.size() - 1)
                        {
                            ImGui::GetWindowDrawList()->AddRectFilled({xpos - 52 * ui_scale, ypos + 17 * ui_scale}, //
                                                                      {xpos - 30 * ui_scale, ypos + 32 * ui_scale}, //
                                                                      (ImU32)((ImColor)c));

                            ImGui::SameLine();
                            ImGui::SetCursorPosX(xpos - 55 * ui_scale);
                            ImGui::Text(u8"\ueaf3");
                            if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
                                std::swap(subhandlers[i], subhandlers[i + 1]);
                        }
                    }

                    handler->drawTreeMenu(h);
                    ImGui::TreePop();
                }
            }
            tree_local.end();

            // Try handle deletion
            delSubHandlersNow();
        }

        bool Handler::hasSubhandlers()
        {
            subhandlers_mtx.lock();
            bool has = subhandlers.size();
            subhandlers_mtx.unlock();
            return has;
        }

        void Handler::addSubHandler(std::shared_ptr<Handler> handler, bool ontop)
        {
            subhandlers_mtx.lock();
            if (ontop)
                subhandlers.insert(subhandlers.begin(), &handler, &handler + 1);
            else
                subhandlers.push_back(handler);
            subhandlers_mtx.unlock();
        }

        void Handler::delSubHandler(std::shared_ptr<Handler> handler, bool now)
        {
            subhandlers_mtx.lock();
            subhandlers_marked_for_del.push_back(handler);
            subhandlers_mtx.unlock();

            if (now)
                delSubHandlersNow();
        }

        std::vector<std::shared_ptr<Handler>> Handler::getAllSubHandlers()
        {
            std::vector<std::shared_ptr<Handler>> sh;
            subhandlers_mtx.lock();
            sh = subhandlers;
            subhandlers_mtx.unlock();
            return sh;
        }

        nlohmann::json Handler::getConfig() { return {}; }
    } // namespace handlers
} // namespace satdump