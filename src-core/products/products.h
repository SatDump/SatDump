#pragma once

#include "nlohmann/json.hpp"
#include "common/image/image.h"
#include "common/tracking/tle.h"

#define PRODUCTS_LOADER_FUN(TYPE) [](std::string file) -> std::shared_ptr<Products> { std::shared_ptr<TYPE> products = std::make_shared<TYPE>(); products->load(file); return products; }

namespace satdump
{
    class Products
    {
    public: // protected:
        nlohmann::json contents;
        bool _has_tle = false;
        TLE tle;

    public:
        std::string instrument_name;
        std::string type;

        void set_tle(std::optional<TLE> tle)
        {
            if (tle.has_value())
            {
                _has_tle = true;
                this->tle = tle.value();
            }
        }

        bool has_tle()
        {
            return _has_tle;
        }

        TLE get_tle()
        {
            return tle;
        }

    public:
        void set_product_timestamp(time_t timestamp) { contents["product_timestamp"] = timestamp; }
        bool has_product_timestamp() { return contents.contains("product_timestamp"); }
        time_t get_product_timestamp() { return contents["product_timestamp"].get<time_t>(); }

        void set_product_source(std::string source) { contents["product_source"] = source; }
        bool has_product_source() { return contents.contains("product_source"); }
        std::string get_product_source() { return contents["product_source"].get<std::string>(); }

    public:
        virtual void save(std::string directory);
        virtual void load(std::string file);
    };

    std::shared_ptr<Products> loadProducts(std::string path);

    // Registry stuff
    struct RegisteredProducts
    {
        std::function<std::shared_ptr<Products>(std::string)> loadFromFile;
        std::function<void(Products *, std::string)> processProducts;
    };

    extern std::map<std::string, RegisteredProducts> products_loaders;

    struct RegisterProductsEvent
    {
        std::map<std::string, RegisteredProducts> &products_loaders;
    };

    void registerProducts();
}