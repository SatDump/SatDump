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

            bool use_preset_cache = p->d_use_preset_cache;

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
                        // If using preset cache, don't redo those we did already
                        if (use_preset_cache)
                        {
                            if (p->contents.contains("preset_cache") &&                                 //
                                p->contents["preset_cache"].contains(cfg["name"].get<std::string>()) && //
                                p->contents["preset_cache"][cfg["name"].get<std::string>()].get<bool>())
                            {
                                // logger->critical("SKIP COMPOSITE " + cfg["name"].get<std::string>());
                                continue;
                            }
                        }

                        handler->resetConfig();
                        handler->setConfig(cfg);
                        handler->process();
                        bool valid = handler->saveResult(directory);

                        // Mark as done if using cache on success
                        if (use_preset_cache && valid)
                            p->contents["preset_cache"][cfg["name"].get<std::string>()] = true;
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