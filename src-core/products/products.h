#pragma once

#include "nlohmann/json.hpp"
#include "common/image/image.h"

namespace satdump
{
    enum InstrumentType
    {

    };

    class Products
    {
    public: // protected:
        nlohmann::json contents;

    public:
        std::string instrument_name;
        std::string type;

    public:
        virtual void save(std::string directory);
        virtual void load(std::string file);
    };

    std::shared_ptr<Products> loadProducts(std::string path);
}