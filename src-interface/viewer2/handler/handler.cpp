#include "handler.h"
#include "imgui/imgui.h"

//// TODOREWORK?
#include "product/image_product_handler.h"

namespace satdump
{
    namespace viewer
    {
        void Handler::drawTreeMenu(std::shared_ptr<Handler> &h)
        {
            // TODOREWORK CLEANUP
            struct DragDropWip
            {
                std::shared_ptr<Handler> drag;
                std::function<void()> del;
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
                    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
                    {
                        auto del = [this, handler]()
                        {
                            logger->error("Delete!!!!");
                            delSubHandler(handler);
                        };
                        DragDropWip *d = new DragDropWip({handler, del});
                        ImGui::SetDragDropPayload("VIEWER_HANDLER", d, sizeof(DragDropWip));

                        ImGui::Text("%s", handler->getName().c_str());
                        ImGui::EndDragDropSource();
                    }

                    if (ImGui::BeginDragDropTarget())
                    {
                        if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("VIEWER_HANDLER"))
                        {
                            IM_ASSERT(payload->DataSize == sizeof(DragDropWip));
                            const DragDropWip *payload_n = (const DragDropWip *)payload->Data;
                            logger->info("Handler " + handler->getName() + " got drag");
                            handler->addSubHandler(payload_n->drag);
                            payload_n->del();
                            delete payload_n;
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
            subhandlers_mtx.lock();
            if (subhandlers_marked_for_del.size())
            {
                for (auto &h : subhandlers_marked_for_del)
                    subhandlers.erase(std::find(subhandlers.begin(), subhandlers.end(), h));
                subhandlers_marked_for_del.clear();
            }
            subhandlers_mtx.unlock();
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

        void Handler::delSubHandler(std::shared_ptr<Handler> handler)
        {
            subhandlers_mtx.lock();
            subhandlers_marked_for_del.push_back(handler);
            subhandlers_mtx.unlock();
        }

        std::map<std::string, std::function<std::shared_ptr<Handler>()>> viewer_handlers_registry;

        void registerHandlers()
        {
            viewer_handlers_registry.emplace(ImageProductHandler::getID(), ImageProductHandler::getInstance);
            //   viewer_handlers_registry.emplace(RadiationViewerHandler::getID(), RadiationViewerHandler::getInstance);
            //   viewer_handlers_registry.emplace(ScatterometerViewerHandler::getID(), ScatterometerViewerHandler::getInstance);
        }
    }
}