#include "product_process.h"
#include "logger.h"

#include "handlers/product/image_product_handler.h"
#include "handlers/product/product_handler.h"
#include "handlers/product/punctiform_product_handler.h"
#include <exception>

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
                    // TODOREWORK REMOVE THIS ASAP!!!!!!
                    if (!cfg.contains("autogen"))
                        continue;
                    if (!cfg["autogen"].get<bool>())
                        continue;

                    try
                    {
                        handler->resetConfig();
                        handler->setConfig(cfg);
                        handler->process();
                        handler->saveResult(directory);
                    }
                    catch (std::exception &e)
                    {
                        logger->error("Error generating preset! %s", e.what());
                    }
                }
            }
        }
    } // namespace products
} // namespace satdump