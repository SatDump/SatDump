#pragma once

#include "../handler.h"

#include "TextEditor.h"

#include "dataset_handler.h"

namespace satdump
{
    namespace viewer
    {
        class DatasetProductProcessor
        {
        protected:
            const DatasetHandler *dataset_handler;
            const Handler *dataset_product_handler;

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
            DatasetProductProcessor(DatasetHandler *dh, Handler *dp)
                : dataset_handler(dh), dataset_product_handler(dp)
            {
            }

            virtual bool can_process() = 0;
            virtual void process(float *progress = nullptr) = 0;
        };

        class DatasetProductHandler : public Handler
        {
        public:
            DatasetProductHandler();
            ~DatasetProductHandler();

            DatasetHandler *dataset_handler;

            int selected_tab = 0;

            TextEditor editor;
            void run_lua();

            // The Rest
            void drawMenu();
            void drawContents(ImVec2 win_size);

            std::string getName() { return "Products"; }

            static std::string getID() { return "dataset_product_handler"; }
            static std::shared_ptr<Handler> getInstance() { return std::make_shared<DatasetProductHandler>(); }
        };
    }
}