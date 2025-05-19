#include "product_handler.h"

#include "logger.h"

#include "handlers/product/image_product_handler.h"
#include "handlers/product/punctiform_product_handler.h"

namespace satdump
{
    namespace handlers
    {
        std::shared_ptr<ProductHandler> getProductHandlerForProduct(std::shared_ptr<products::Product> prod)
        {
            std::shared_ptr<ProductHandler> prod_h;

            if (prod->type == "image")
                prod_h = std::make_shared<ImageProductHandler>(prod, true);
            else if (prod->type == "punctiform")
                prod_h = std::make_shared<PunctiformProductHandler>(prod, true);
            else
                logger->error("TODOREWORK!!!!!! Actually handle loading properly...");

            return prod_h;
        }
    } // namespace handlers
} // namespace satdump