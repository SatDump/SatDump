#pragma once

#include "core/config.h"
#include "core/plugin.h"
#include "products/product.h"
#include <filesystem>
#include <random>
#include <string>

namespace satdump_test_products
{
    // Unique temporary directory, removed with its contents on scope exit
    struct TempDir
    {
        std::filesystem::path path;

        explicit TempDir(const std::string &tag)
        {
            std::mt19937_64 rng(std::random_device{}());
            path = std::filesystem::temp_directory_path() / ("satdump_test_products_" + tag + "_" + std::to_string(rng()));
            std::filesystem::create_directories(path);
        }
        TempDir(const TempDir &) = delete;
        TempDir &operator=(const TempDir &) = delete;
        ~TempDir()
        {
            std::error_code ec;
            std::filesystem::remove_all(path, ec);
        }

        std::string str() const { return path.string(); }
    };

    // Saves and restores the global product loader registry
    struct ProductLoadersGuard
    {
        std::map<std::string, satdump::products::RegisteredProduct> saved;
        ProductLoadersGuard() : saved(satdump::products::product_loaders) {}
        ProductLoadersGuard(const ProductLoadersGuard &) = delete;
        ProductLoadersGuard &operator=(const ProductLoadersGuard &) = delete;
        ~ProductLoadersGuard() { satdump::products::product_loaders = saved; }
    };

    // Saves and restores the global SatDump config. Several product functions read it with
    // operator[], which inserts null keys, so even "read-only" use has to be undone.
    struct ConfigGuard
    {
        nlohmann::ordered_json saved;
        ConfigGuard() : saved(satdump::satdump_cfg.main_cfg) {}
        ConfigGuard(const ConfigGuard &) = delete;
        ConfigGuard &operator=(const ConfigGuard &) = delete;
        ~ConfigGuard() { satdump::satdump_cfg.main_cfg = saved; }

        void setProductFormat(const std::string &fmt) { satdump::satdump_cfg.main_cfg["satdump_general"]["product_format"]["value"] = fmt; }
    };

    // Swaps in a fresh event bus so handlers registered by a test disappear afterwards
    struct EventBusGuard
    {
        std::shared_ptr<satdump::EventBus> saved;
        EventBusGuard() : saved(satdump::eventBus) { satdump::eventBus = std::make_shared<satdump::EventBus>(); }
        EventBusGuard(const EventBusGuard &) = delete;
        EventBusGuard &operator=(const EventBusGuard &) = delete;
        ~EventBusGuard() { satdump::eventBus = saved; }
    };
} // namespace satdump_test_products
