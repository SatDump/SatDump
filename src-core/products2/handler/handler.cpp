#include "handler.h"
#include "imgui/imgui.h"

//// TODOREWORK?
#include "product/image_product_handler.h"

namespace satdump
{
    namespace viewer
    {
        inline ImGuiTreeNodeFlags nodeFlags(std::shared_ptr<Handler> &h, bool sec)
        {
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
            if (!h->hasSubhandlers())
                flags |= ImGuiTreeNodeFlags_Leaf;
            if (sec)
                flags |= ImGuiTreeNodeFlags_Selected;
            return flags;
        }

        void Handler::drawTreeMenu(std::shared_ptr<Handler> &h)
        {
            // TODOREWORK CLEANUP
            struct DragDropWip
            {
                std::shared_ptr<Handler> drag;
                std::function<void(std::shared_ptr<Handler>)> del;
            };

            tree_local.start();
            for (int i = 0; i < subhandlers.size(); i++)
            {
                auto &handler = subhandlers[i];
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);

                if (ImGui::TreeNodeEx(handler->getTreeID().c_str(), nodeFlags(handler, h == handler)) |
                    tree_local.node())
                {
                    if (ImGui::IsItemClicked())
                        h = handler;

                    // TODOREWORK CLEANUP
                    if (handler_can_subhandlers_be_dragged && handler->handler_can_be_dragged &&
                        ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
                    {
                        auto del = [this](std::shared_ptr<Handler> handler)
                        {
                            logger->error("Delete!!!! " + handler->getName());
                            delSubHandler(handler);
                        };
                        DragDropWip d({handler, del});
                        ImGui::SetDragDropPayload("VIEWER_HANDLER", &d, sizeof(DragDropWip));

                        ImGui::Text("%s", handler->getName().c_str());
                        ImGui::EndDragDropSource();
                    }

                    if (handler->handler_can_be_dragged_to && ImGui::BeginDragDropTarget())
                    {
                        if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("VIEWER_HANDLER"))
                        {
                            IM_ASSERT(payload->DataSize == sizeof(DragDropWip));
                            const DragDropWip *payload_n = (const DragDropWip *)payload->Data;
                            logger->info("Handler " + handler->getName() + " got drag of " + payload_n->drag->getName());
                            handler->addSubHandler(payload_n->drag);
                            payload_n->del(payload_n->drag);
                        }
                        ImGui::EndDragDropTarget();
                    }
                    // TODOREWORK CLEANUP

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

        void Handler::addSubHandler(std::shared_ptr<Handler> handler)
        {
            subhandlers_mtx.lock();
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

        nlohmann::json Handler::getConfig()
        {
            return {};
        }
    }
}