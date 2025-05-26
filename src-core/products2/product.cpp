#include "product.h"
#include "utils/http.h"
#include "core/exception.h"
#include "core/plugin.h"
#include "logger.h"
#include <filesystem>
#include <fstream>

#include "image_product.h"
#include "punctiform_product.h"

namespace satdump
{
    namespace products
    {
        void Product::save(std::string directory)
        {
            contents["instrument"] = instrument_name;
            contents["type"] = type;

            // Write the file out
            std::vector<uint8_t> cbor_data = nlohmann::json::to_cbor(contents);
            std::ofstream out_file(directory + "/product.cbor", std::ios::binary);
            out_file.write((char *)cbor_data.data(), cbor_data.size());
            out_file.close();
        }

        void Product::load(std::string file)
        {
            // Read file
            std::vector<uint8_t> cbor_data;
            if (file.find("http") == 0)
            {
                std::string res;
                if (perform_http_request(file, res))
                    throw satdump_exception("Could not download from : " + file);
                for (auto &v : res)
                    cbor_data.push_back(*((uint8_t *)&v));
            }
            else
            {
                if (!std::filesystem::exists(file))
                    throw satdump_exception("Bad file : " + file);
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
        }

        std::shared_ptr<Product> loadProduct(std::string path)
        {
            std::shared_ptr<Product> final_products;
            Product raw_products;

            if (std::filesystem::is_directory(path) || (path.find("http") == 0 && path.find(".cbor") == std::string::npos))
                path = path + "/product.cbor";

            raw_products.load(path);

            if (product_loaders.count(raw_products.type) > 0)
                return product_loaders[raw_products.type].loadFromFile(path);
            else
            {
                final_products = std::make_shared<Product>();
                final_products->load(path);
                return final_products;
            }
        }

        std::map<std::string, RegisteredProduct> product_loaders;

        void registerProducts()
        {
            product_loaders.clear(); /*TODOREWORK*/
            product_loaders.emplace("image", RegisteredProduct{PRODUCT_LOADER_FUN(ImageProduct)});
            product_loaders.emplace("punctiform", RegisteredProduct{PRODUCT_LOADER_FUN(PunctiformProduct)});
            //  products_loaders.emplace("radiation", RegisteredProducts{PRODUCTS_LOADER_FUN(RadiationProducts), process_radiation_products}); TODOREWORK?
            //  products_loaders.emplace("scatterometer", RegisteredProducts{PRODUCTS_LOADER_FUN(ScatterometerProducts), process_scatterometer_products});

            // Plugins!
            eventBus->fire_event<RegisterProductsEvent>({product_loaders});
        }
    } // namespace products
} // namespace satdump