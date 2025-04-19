#include "product_process.h"
#include "logger.h"

#include "handler/product/product_handler.h"
#include "handler/product/image_product_handler.h"
#include "handler/product/punctiform_product_handler.h"

namespace satdump
{
    namespace products
    { // TODOREWORK?
        void process_product_with_handler(std::shared_ptr<Product> p, std::string directory)
        {
            std::shared_ptr<viewer::ProductHandler> handler;
            if (p->type == "image")
                handler = std::make_shared<viewer::ImageProductHandler>(p);
            else if (p->type == "punctiform")
                handler = std::make_shared<viewer::PunctiformProductHandler>(p);
            else
                logger->critical("TODOREWORK");

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
    }
}