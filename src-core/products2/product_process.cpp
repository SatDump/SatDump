#include "product_process.h"
#include "logger.h"

#include "handlers/product/image_product_handler.h"
#include "handlers/product/product_handler.h"
#include "handlers/product/punctiform_product_handler.h"

namespace satdump
{
    namespace products
    { // TODOREWORK?
        void process_product_with_handler(std::shared_ptr<Product> p, std::string directory)
        {
            std::shared_ptr<handlers::ProductHandler> handler = handlers::getProductHandlerForProduct(p);

            auto instr_cfg = handler->getInstrumentCfg();
            if (instr_cfg.contains("presets"))
            {
                for (auto &cfg : instr_cfg["presets"])
                {
                    handler->setConfig(cfg);
                    handler->process();
                    handler->saveResult(directory);
                }
            }
        }
    } // namespace products
} // namespace satdump