#pragma once

#include "nlohmann/json.hpp"

namespace satdump
{
    struct ProductDataSet
    {
        std::string satellite_name;             // Source satellite name. Should be descriptive enough, but if the specific satellite (eg, MetOp-B) is unknown, "MetOp" is enough
        double timestamp;                       // Rough UTC Timestamp of the dataset
        std::vector<std::string> products_list; // List of products to load, relative to this file

        void save(std::string path);
        void load(std::string path);
    };
}
