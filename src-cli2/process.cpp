#include "process.h"
#include "products/product.h"
#include "products/product_process.h"
#include <filesystem>

namespace satdump
{
    void ProcessCmdHandler::reg(CLI::App *app)
    {
        CLI::App *sub_process = app->add_subcommand("process", "Process products/dataset automatically");
        sub_process->add_option("product", "Product to process")->required();
        sub_process->add_option("directory", "Output folder")->required();
    }

    void ProcessCmdHandler::run(CLI::App *app, CLI::App *subcom)
    {
        std::string pro = subcom->get_option("product")->as<std::string>();
        std::string dir = subcom->get_option("directory")->as<std::string>();
        processProductsOrDataset(pro, dir);
    }

    void ProcessCmdHandler::processProductsOrDataset(std::string product, std::string directory)
    {
        if (!std::filesystem::exists(directory))
            std::filesystem::create_directories(directory);

        auto p = products::loadProduct(product);
        satdump::products::process_product_with_handler(p, directory);
    }
} // namespace satdump