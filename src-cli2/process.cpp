#include "process.h"
#include "products2/product.h"
#include "products2/product_process.h"
#include <filesystem>

namespace satdump
{
    void processProductsOrDataset(std::string product, std::string directory)
    {
        if (!std::filesystem::exists(directory))
            std::filesystem::create_directories(directory);

        auto p = products::loadProduct(product);
        satdump::products::process_product_with_handler(p, directory);
    }
} // namespace satdump