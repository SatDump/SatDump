#pragma once

/**
 * @file product_handler.h
 */

#include "../handler.h"
#include "../processing_handler.h"
#include "common/widgets/markdown_helper.h"
#include "nlohmann/json.hpp"
#include "products/product.h"
#include <functional>
#include <memory>
#include <vector>

namespace satdump
{
    namespace handlers
    {
        /**
         * @brief Product handler base class.
         *
         * A common usecase of handlers in the explorer is to allow
         * displaying and doing basic manipulation on instrument products.
         *
         * To this effect, this base class implements the common interface
         * for handling those instrument products.
         *
         * @param product raw product pointer, to be cast in the inheriting class
         * @param instrument_cfg TODOREWORK instrument configuration file, for presets
         */
        class ProductHandler : public Handler, public ProcessingHandler
        {
        private:
            std::string handler_name;

            std::string preset_search_str;
            std::vector<nlohmann::json> all_presets;
            std::vector<std::string> preset_selection_box_str;
            int preset_selection_curr_id = -1;

            bool has_markdown_description = false;
            bool show_markdown_description = false;
            widgets::MarkdownHelper markdown_info;

        protected:
            // TODOREWORK document
            bool preset_reset_by_handler = false;
            void resetPreset()
            {
                preset_selection_curr_id = -1;
                has_markdown_description = false;
            }

        protected:
            std::shared_ptr<products::Product> product;
            nlohmann::ordered_json instrument_cfg;

            // TODOREWORK document!
            std::string product_internal_name; // TODOREWORK? Rename? No idea
            std::string generateFileName();

        protected:
            /**
             * @brief Draw preset selection menu
             * @return if one was selected
             */
            bool renderPresetMenu();

            /**
             * @brief Attempts to a apply a default preset if present in the
             * configuration
             */
            void tryApplyDefaultPreset();

        public:
            nlohmann::json getInstrumentCfg() { return instrument_cfg; }
            virtual void saveResult(std::string directory);

        public:
            /**
             * @brief Constructor
             * @param p product to handle
             * @param dataset_mode if true, only displays the instrument name,
             * otherwise adds timestamp and source name for clarity
             * @param filterPreset optional function to filter the presets list
             */
            ProductHandler(std::shared_ptr<products::Product> p, bool dataset_mode = false, std::function<bool(nlohmann::ordered_json &)> filterPreset = [](auto &) { return true; });

            std::string getName() { return handler_name; }

            std::string getID() { return "product_handler"; };
        };

        /**
         * @brief Event used to let plugins provide additional
         * ProductHandlers.
         *
         * @param product the product
         * @param handler the handler, nullptr by default
         * @param dataset_mode to forward to the created handler
         * as needed
         */
        struct RequestProductHandlerEvent
        {
            std::shared_ptr<products::Product> &product;
            std::shared_ptr<ProductHandler> &handler;
            bool dataset_mode;
        };

        /**
         * @brief Get the appropriate ProductHandler for the
         * provided products.
         *
         * @param product product, loaded with loadProducts()
         * @param dataset_mode must be true if opening inside a dataset
         * @return appropriate handler pointer
         */
        std::shared_ptr<ProductHandler> getProductHandlerForProduct(std::shared_ptr<products::Product> product, bool dataset_mode = false);
    } // namespace handlers
} // namespace satdump