#include "processor.h"
#include "logger.h"
#include "../dataset.h"
#include "../image_products.h"
#include "core/config.h"
#include <filesystem>

namespace satdump
{
    void process_product(std::string product_path)
    {
        logger->info(product_path);
        std::shared_ptr<Products> products = loadProducts(product_path);

        if (products_loaders.count(products->type) > 0)
            products_loaders[products->type].processProducts(products.get(), product_path);
        else
            logger->error("No handler for type " + products->type);
    }

    void process_dataset(std::string dataset_path)
    {
        ProductDataSet dataset;
        dataset.load(dataset_path);

        std::string pro_directory = std::filesystem::path(dataset_path).parent_path().string();
        for (std::string pro_path : dataset.products_list)
        {
            process_product(pro_directory + "/" + pro_path);
        }
    }
}