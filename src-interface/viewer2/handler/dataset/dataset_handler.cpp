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
            instrument_products->setCanBeDragged(false);
            instrument_products->setCanBeDraggedTo(false);
            instrument_products->setSubHandlersCanBeDragged(false);
            general_products->setCanBeDragged(false);
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
