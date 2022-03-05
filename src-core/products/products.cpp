#include "products.h"
#include <fstream>
#include "logger.h"

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

    void ImageProducts::save(std::string directory)
    {
        contents["has_timestamps"] = has_timestamps;

        for (size_t c = 0; c < images.size(); c++)
        {
            contents["images"][c] = images[c].first;
            logger->info("Saving " + images[c].first);
            images[c].second.save_png(directory + "/" + images[c].first);
        }

        Products::save(directory);
    }
}