#include "products.h"
#include <fstream>
#include "logger.h"
#include <filesystem>
#include "image_products.h"
#include "radiation_products.h"
#include "scatterometer_products.h"
#include "core/plugin.h"
#include "processor/image_processor.h"
#include "processor/radiation_processor.h"
#include "processor/scatterometer_processor.h"
#include "common/utils.h"

namespace satdump
{
    void Products::save(std::string directory)
    {
        contents["instrument"] = instrument_name;
        contents["type"] = type;

        if (_has_tle)
            contents["tle"] = tle;

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
        if (file.find("http") == 0)
        {
            std::string res;
            if (perform_http_request(file, res))
                throw std::runtime_error("Could not download from : " + file);
            for (auto &v : res)
                cbor_data.push_back(*((uint8_t *)&v));
        }
        else
        {
            std::ifstream in_file(file, std::ios::binary);
            while (!in_file.eof())
            {
                uint8_t b;
                in_file.read((char *)&b, 1);
                cbor_data.push_back(b);
            }
            in_file.close();
            cbor_data.pop_back();
        }
        contents = nlohmann::json::from_cbor(cbor_data);

        instrument_name = contents["instrument"].get<std::string>();
        type = contents["type"].get<std::string>();

        _has_tle = contents.contains("tle");
        if (_has_tle)
            tle = contents["tle"].get<TLE>();
    }

    std::shared_ptr<Products> loadProducts(std::string path)
    {
        std::shared_ptr<Products> final_products;
        Products raw_products;

        if (std::filesystem::is_directory(path) ||
            (path.find("http") == 0 && path.find(".cbor") == std::string::npos))
            path = path + "/product.cbor";

        raw_products.load(path);

        if (products_loaders.count(raw_products.type) > 0)
            return products_loaders[raw_products.type].loadFromFile(path);
        else
        {
            final_products = std::make_shared<Products>();
            final_products->load(path);
            return final_products;
        }
    }

    std::map<std::string, RegisteredProducts> products_loaders;

    void registerProducts()
    {
        products_loaders.clear();
        products_loaders.emplace("image", RegisteredProducts{PRODUCTS_LOADER_FUN(ImageProducts), process_image_products});
        products_loaders.emplace("radiation", RegisteredProducts{PRODUCTS_LOADER_FUN(RadiationProducts), process_radiation_products});
        products_loaders.emplace("scatterometer", RegisteredProducts{PRODUCTS_LOADER_FUN(ScatterometerProducts), process_scatterometer_products});

        // Plugins!
        eventBus->fire_event<RegisterProductsEvent>({products_loaders});
    }
}