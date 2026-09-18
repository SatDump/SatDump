#pragma once

#include "core/exception.h"
#include "core/plugin.h"
#include "explorer/explorer.h"
#include "handlers/dataset/dataset_product_handler.h"
#include "handlers/dataset/flowgraph/flowgraph.h"
#include "handlers/handler.h"
#include <memory>

namespace satdump
{
    class DatasetProductSource_Node : public NodeInternal
    {
    private:
        handlers::Handler *handler;
        std::string product_id;
        int product_index = 0;

    public:
        DatasetProductSource_Node(handlers::Handler *handler) : NodeInternal("Dataset Product Source"), handler(handler) { outputs.push_back({"Product", "product"}); }

        void process()
        {
            std::shared_ptr<handlers::Handler> p;
            std::shared_ptr<handlers::Handler> h(handler, [](handlers::Handler *) {});
            eventBus->fire_event<explorer::GetParentOfHandlerEvent>({h, p});
            if (p && p->getID() == "dataset_product_handler")
            {
                handlers::DatasetProductHandler *proc = ((handlers::DatasetProductHandler *)p.get());
                outputs[0].ptr = std::shared_ptr<products::Product>(proc->get_instrument_products(product_id, product_index), [](products::Product *) {}); // No Deleter
                has_run = true;
            }
            else
            {
                throw satdump_exception("Must be below a dataset product handler!");
            }
        }

        void render()
        {
            ImGui::SetNextItemWidth(200 * ui_scale);
            ImGui::InputText("ID", &product_id);
            ImGui::SetNextItemWidth(200 * ui_scale);
            ImGui::InputInt("Index", &product_index);
        }

        nlohmann::json to_json()
        {
            nlohmann::json j;
            j["id"] = product_id;
            j["index"] = product_index;
            return j;
        }

        void from_json(nlohmann::json j)
        {
            product_id = j["id"];
            product_index = j["index"];
        }
    };
} // namespace satdump