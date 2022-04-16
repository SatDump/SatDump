#pragma once

#include "nlohmann/json.hpp"
#include "nlohmann/json_utils.h"

namespace satdump
{
    struct ProductDataSet
    {
        std::string satellite_name;             // Source satellite name. Should be descriptive enough, but if the specific satellite (eg, MetOp-B) is unknown, "MetOp" is enough
        double timestamp;                       // Rough UTC Timestamp of the dataset
        std::vector<std::string> products_list; // List of products to load, relative to this file

        inline void save(std::string path)
        {
            nlohmann::json json_ojb;
            json_ojb["satellite"] = satellite_name;
            json_ojb["timestamp"] = timestamp;
            json_ojb["products"] = products_list;

            saveJsonFile(path + "/dataset.json", json_ojb);
        }

        inline void load(std::string path)
        {
            nlohmann::json json_ojb = loadJsonFile(path);
            satellite_name = json_ojb["satellite"].get<std::string>();
            timestamp = json_ojb["timestamp"].get<double>();
            products_list = json_ojb["products"].get<std::vector<std::string>>();
        }
    };
}
