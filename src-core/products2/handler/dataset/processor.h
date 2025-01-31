#pragma once

#include "dataset_handler.h"
#include "core/params.h"

namespace satdump
{
    namespace viewer
    {
        class DatasetProductProcessor
        {
        protected:
            const DatasetHandler *dataset_handler;
            const Handler *dataset_product_handler;
            //            const nlohmann::json parameters;

            std::map<std::string, params::EditableParameter> params; // TODOREWORK make this more dynamic...

        protected:
            nlohmann::json getParams()
            {
                nlohmann::json cfg;
                for (auto &p : params)
                    cfg[p.first] = p.second.getValue();
                return cfg;
            }

        protected:
            products::Product *get_instrument_products(std::string v, int index)
            {
                std::vector<products::Product *> pro;
                for (auto &h : dataset_handler->all_products)
                    if (h->instrument_name == v)
                        pro.push_back(h.get());
                return index < pro.size() ? pro[index] : nullptr;
            };

            void add_handler_to_products(std::shared_ptr<Handler> p)
            {
                if (p)
                    ((Handler *)dataset_product_handler)->addSubHandler(p);
            };

        public:
            DatasetProductProcessor(DatasetHandler *dh, Handler *dp, nlohmann::json p)
                : dataset_handler(dh), dataset_product_handler(dp) //, parameters(p)
            {
                if (p.contains("cfg"))
                    params = p["cfg"];
            }

            virtual nlohmann::json getCfg() = 0;

            virtual bool can_process() = 0;
            virtual void process(float *progress = nullptr) = 0;
            virtual void renderUI() = 0;

            void renderParams()
            {
                if (ImGui::BeginTable("##pipelinesmainoptions", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders))
                {
                    ImGui::TableSetupColumn("##pipelinesmaincolumn1", ImGuiTableColumnFlags_None);
                    ImGui::TableSetupColumn("##pipelinesmaincolumn2", ImGuiTableColumnFlags_None);

                    for (auto &p : params)
                        p.second.draw();

                    ImGui::EndTable();
                }
            }
        };
    }
}