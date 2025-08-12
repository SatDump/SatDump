#include "common/repack.h"
#include "core/resources.h"
#include "explorer/explorer.h"
#include "handlers/product/image_product_handler.h"
#include "handlers/product/product_handler.h"
#include "nlohmann/json.hpp"
#include "nlohmann/json_utils.h"
#include "procfile.h"
#include "products/image_product.h"
#include "products/product.h"
#include "projection/standard/proj.h"
#include "projection/standard/proj_json.h"
#include "type.h"
#include "utils/file/file_iterators.h"
#include <cstdint>
#include <string>

void provideExplorerFileLoader(const satdump::explorer::ExplorerRequestFileLoad &evt)
{
    auto info = satdump::firstparty::identifyFirstPartyProductFile(evt.path);

    if (info.type != satdump::firstparty::PRODUCT_NONE)
    {
        logger->info("Identified first-party product : " + info.name + " (" + evt.path + ")");

        evt.loaders.push_back({"First-Party Products", [info](std::string p, satdump::explorer::ExplorerApplication *e)
                               {
                                   auto products = satdump::firstparty::processFirstPartyProductFile(info, p);
                                   e->addHandler(satdump::handlers::getProductHandlerForProduct(products));
                               }});

        logger->trace("MSG SEVIRI Images");
    }
}