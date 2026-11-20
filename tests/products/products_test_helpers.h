#pragma once

#include "core/config.h"
#include "helpers/satdump_guards.h"
#include "helpers/temp_files.h"
#include "products/product.h"
#include <map>
#include <string>

namespace products_test
{
    using test_helpers::ConfigGuard;
    using test_helpers::EventBusGuard;
    using test_helpers::TempDir;

    // Sets the product format the save functions use. Call with a ConfigGuard alive
    inline void setProductFormat(const std::string &fmt) { satdump::satdump_cfg.main_cfg["satdump_general"]["product_format"]["value"] = fmt; }

    // Saves and restores the global product loader registry
    struct ProductLoadersGuard
    {
        std::map<std::string, satdump::products::RegisteredProduct> saved;
        ProductLoadersGuard() : saved(satdump::products::product_loaders) {}
        ProductLoadersGuard(const ProductLoadersGuard &) = delete;
        ProductLoadersGuard &operator=(const ProductLoadersGuard &) = delete;
        ~ProductLoadersGuard() { satdump::products::product_loaders = saved; }
    };
} // namespace products_test
