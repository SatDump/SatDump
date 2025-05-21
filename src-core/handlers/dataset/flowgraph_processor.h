#pragma once

#include "flowgraph/flowgraph.h"
#include "processor.h"

namespace satdump
{
    namespace handlers
    {
        class Flowgraph_DatasetProductProcessor : public DatasetProductProcessor
        {
        private:
            Flowgraph flowgraph;

        private: // TODOREWORK?
            class DatasetProductSource_Node : public NodeInternal
            {
            private:
                Flowgraph_DatasetProductProcessor *proc;
                std::string product_id;
                int product_index = 0;

            public:
                DatasetProductSource_Node(Flowgraph_DatasetProductProcessor *proc) : NodeInternal("Dataset Product Source"), proc(proc) { outputs.push_back({"Product"}); }

                void process()
                {
                    outputs[0].ptr = std::shared_ptr<products::Product>(proc->get_instrument_products(product_id, product_index), [](products::Product *) {}); // No Deleter
                    has_run = true;
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

        public:
            Flowgraph_DatasetProductProcessor(DatasetHandler *dh, Handler *dp, nlohmann::json p);

            nlohmann::json getCfg();

            bool can_process();
            void process(float *progress = nullptr);
            void renderUI();
        };
    } // namespace handlers
} // namespace satdump