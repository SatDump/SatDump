#include "products.h"
#include <fstream>
#include "logger.h"
#include <filesystem>
#include "image_products.h"

namespace satdump
{
    void Products::save(std::string directory)
    {
        contents["instrument"] = instrument_name;
        contents["type"] = type;

        // Write the file out
        std::vector<uint8_t> cbor_data = nlohmann::json::to_cbor(contents);
        std::ofstream out_file(directory + "/product.cbor", std::ios::binary);
        out_file.write((char *)cbor_data.data(), cbor_data.size());
        out_file.close();
    }

    void Products::load(std::string file)
    {
        logger->info(file);
        // Read file
        std::vector<uint8_t> cbor_data;
        std::ifstream in_file(file, std::ios::binary);
        while (!in_file.eof())
        {
            uint8_t b;
            in_file.read((char *)&b, 1);
            cbor_data.push_back(b);
        }
        in_file.close();
        cbor_data.pop_back();
        contents = nlohmann::json::from_cbor(cbor_data);

        instrument_name = contents["instrument"].get<std::string>();
        type = contents["type"].get<std::string>();
    }

    std::shared_ptr<Products> loadProducts(std::string path)
    {
        std::shared_ptr<Products> final_products;
        Products raw_products;

        raw_products.load(path);

        if (raw_products.type == "image")
            final_products = std::make_shared<ImageProducts>();
        else
            final_products = std::make_shared<Products>();

        final_products->load(path);
        return final_products;
    }
}