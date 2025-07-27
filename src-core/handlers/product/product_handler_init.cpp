#include "core/exception.h"
#include "core/plugin.h"
#include "product_handler.h"

#include "logger.h"

#include "handlers/product/image_product_handler.h"
#include "handlers/product/punctiform_product_handler.h"

namespace satdump
{
    namespace handlers
    {
        std::shared_ptr<ProductHandler> getProductHandlerForProduct(std::shared_ptr<products::Product> prod, bool dataset_mode)
        {
            if (!prod)
                throw satdump_exception("Products are null!");

            // Built-in standard handlers
            if (prod->type == "image")
                return std::make_shared<ImageProductHandler>(prod, dataset_mode);
            if (prod->type == "punctiform")
                return std::make_shared<PunctiformProductHandler>(prod, dataset_mode);

            // Plugin stuff, as needed
            std::shared_ptr<ProductHandler> prod_h = nullptr;
            eventBus->fire_event<RequestProductHandlerEvent>({prod, prod_h, dataset_mode});
            if (prod_h)
                return prod_h;
            else
                throw satdump_exception("Couldn't get a handler for product type " + prod->type + " and instrument " + prod->instrument_name + "!");
        }
    } // namespace handlers
} // namespace satdump