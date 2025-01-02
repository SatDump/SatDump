#include "dataset_handler.h"

#include "../dummy_handler.h"
#include "dataset_product_handler.h"

namespace satdump
{
    namespace viewer
    {
        DatasetHandler::DatasetHandler()
        {
            instrument_products = std::make_shared<DummyHandler>("Instruments");
            general_products = std::make_shared<DatasetProductHandler>();
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
