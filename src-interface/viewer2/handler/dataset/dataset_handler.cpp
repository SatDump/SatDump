#include "dataset_handler.h"

#include "../dummy_handler.h"
#include "dataset_product_handler.h"

namespace satdump
{
    namespace viewer
    {
        void DatasetHandler::init()
        {
            instrument_products = std::make_shared<DummyHandler>();
            general_products = std::make_shared<DatasetProductHandler>();
            ((DummyHandler *)instrument_products.get())->name = "Instruments";
            addSubHandler(instrument_products);
            addSubHandler(general_products);
        }

        DatasetHandler::~DatasetHandler()
        {
        }

        void DatasetHandler::drawMenu()
        {
        }

        void DatasetHandler::drawContents(ImVec2 win_size)
        {
        }
    }
}
