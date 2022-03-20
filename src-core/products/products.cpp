#include "products.h"
#include <fstream>
#include "logger.h"
#include <filesystem>

namespace satdump
{
    void Products::save(std::string directory)
    {
        contents["instrument"] = instrument_name;

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
    }

    void ImageProducts::save(std::string directory)
    {
        contents["has_timestamps"] = has_timestamps;

        for (size_t c = 0; c < images.size(); c++)
        {
            contents["images"][c]["file"] = std::get<0>(images[c]);
            contents["images"][c]["name"] = std::get<1>(images[c]);
            logger->info("Saving " + std::get<0>(images[c]));
            std::get<2>(images[c]).save_png(directory + "/" + std::get<0>(images[c]));
        }

        Products::save(directory);
    }

    void ImageProducts::load(std::string file)
    {
        Products::load(file);
        std::string directory = std::filesystem::path(file).parent_path();

        for (size_t c = 0; c < contents["images"].size(); c++)
        {
            image::Image<uint16_t> image;
            logger->info("Loading " + contents["images"][c]["file"].get<std::string>());
            image.load_png(directory + "/" + contents["images"][c]["file"].get<std::string>());
            images.push_back({contents["images"][c]["file"].get<std::string>(), contents["images"][c]["name"].get<std::string>(), image});
        }
    }
}