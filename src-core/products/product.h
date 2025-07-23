#pragma once

/**
 * @file product.h
 * @brief Core Product implementation
 */

#include "nlohmann/json.hpp"

#define PRODUCT_LOADER_FUN(TYPE)                                                                                                                                                                       \
    [](std::string file) -> std::shared_ptr<Product>                                                                                                                                                   \
    {                                                                                                                                                                                                  \
        std::shared_ptr<TYPE> product = std::make_shared<TYPE>();                                                                                                                                      \
        product->load(file);                                                                                                                                                                           \
        return product;                                                                                                                                                                                \
    }

namespace satdump
{
    namespace products
    {
        /**
         * @brief Core SatDump product class.
         *
         * This is the base class of all products, images or anything else.
         * They should ONLY be loaded using the loadProducts() function!
         *
         * @param contents raw JSON product data and/or description. Usually not accessed directly
         * @param instrument_name name (ID) of the instrument that generated the data, or other ID type if needed
         * @param type type of the product, set by the class using this as a base
         */
        class Product
        {
        public: // protected:
            nlohmann::json contents;

        public:
            std::string instrument_name;
            std::string type;

        public:
            Product() {}
            Product(Product const &) = delete;
            void operator=(Product const &x) = delete;

        public:
            /**
             * @brief Set product timestamp, optional. This is usually the rough creation time / acquisition time
             * @param timestamp UNIX timestamp
             */
            void set_product_timestamp(double timestamp) { contents["product_timestamp"] = timestamp; }

            /**
             * @brief Check if a product timestamp is present
             * @return true if present
             */
            bool has_product_timestamp() { return contents.contains("product_timestamp"); }

            /**
             * @brief Get the product timestamp
             * @return UNIX timestamp
             */
            double get_product_timestamp() { return contents["product_timestamp"].get<double>(); }

            /**
             * @brief Set product source, optional. This is meant to contextualize where this product is from, eg which satellite.
             * @param source product source name
             */
            void set_product_source(std::string source) { contents["product_source"] = source; }

            /**
             * @brief Check if a product source is present
             * @return true if present
             */
            bool has_product_source() { return contents.contains("product_source"); }

            /**
             * @brief Get the product source
             * @return source name
             */
            std::string get_product_source() { return contents["product_source"].get<std::string>(); }

            /**
             * @brief Set product ID, optional. This is meant to, for example, differentiate several identical instruments.
             * @param id product ID
             */
            void set_product_id(std::string id) { contents["product_id"] = id; }

            /**
             * @brief Check if a product ID is present
             * @return true if present
             */
            bool has_product_id() { return contents.contains("product_id"); }

            /**
             * @brief Get the product ID
             * @return the ID
             */
            std::string get_product_id() { return contents["product_id"].get<std::string>(); }

        public:
            /**
             * @brief Save the product. Depending on the type this will save
             * a product.cbor and other files in the same directory (eg, images)
             * @param directory directory to save into
             */
            virtual void save(std::string directory);

            /**
             * @brief Load the product. This should refer to the product.cbor file
             * @param file cbor file to load
             */
            virtual void load(std::string file);
        };

        /**
         * @brief Load a product. This will detect the product type
         * and load it appropriately using the registered loader
         * if found. If not, it will return it as the base Product class.
         * @param file cbor file to load
         */
        std::shared_ptr<Product> loadProduct(std::string path);

        /**
         * @brief Struct holding functions related to products
         * @param loadFromFile function to load a specific product type from the cbor file. Eg, simply calls Product::load()
         */
        struct RegisteredProduct
        {
            std::function<std::shared_ptr<Product>(std::string)> loadFromFile;
        };

        /**
         * @brief Registry of product loaders, string id (eg, "image") and RegisteredProduct struct.
         * Should not be used by users! TODOREWORK : Maybe hide?
         */
        extern std::map<std::string, RegisteredProduct> product_loaders;

        /**
         * @brief Event to subscribe to in plugins to insert new product loaders
         * @param product_loaders Registry to insert into
         */
        struct RegisterProductsEvent
        {
            std::map<std::string, RegisteredProduct> &product_loaders;
        };

        /**
         * @brief Function to register product loaders.
         * MUST be called before calling any other product function!
         * @param
         */
        void registerProducts();
    } // namespace products
} // namespace satdump