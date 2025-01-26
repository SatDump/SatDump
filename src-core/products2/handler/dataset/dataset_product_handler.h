#pragma once

#include "../handler.h"

#include "TextEditor.h"

#include "dataset_handler.h"

#include "flowgraph/flowgraph.h"

namespace satdump
{
    namespace viewer
    {
        class DatasetProductHandler : public Handler
        {
        public:
            DatasetProductHandler();
            ~DatasetProductHandler();

            DatasetHandler *dataset_handler;

            int selected_tab = 0;

            TextEditor editor;
            void run_lua();

            Flowgraph flowgraph;

            // The Rest
            void drawMenu();
            void drawContents(ImVec2 win_size);

            std::string getName() { return "Products"; }

            static std::string getID() { return "dataset_product_handler"; }
            static std::shared_ptr<Handler> getInstance() { return std::make_shared<DatasetProductHandler>(); }
        };
    }
}