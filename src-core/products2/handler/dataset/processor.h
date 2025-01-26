#pragma once

#include "dataset_product_handler.h"

namespace satdump
{
    namespace viewer
    {
        class DatasetProductProcessor
        {
        protected:
            const DatasetHandler *dataset_handler;
            const Handler *dataset_product_handler;
            const nlohmann::json parameters;

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
                : dataset_handler(dh), dataset_product_handler(dp), parameters(p)
            {
            }

            virtual bool can_process() = 0;
            virtual void process(float *progress = nullptr) = 0;
        };
    }
}