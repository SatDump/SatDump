#pragma once

#include "nlohmann/json.hpp"
#include "common/image/image.h"
#include "common/tracking/tle.h"

namespace satdump
{
    enum InstrumentType
    {

    };

    class Products
    {
    public: // protected:
        nlohmann::json contents;
        bool _has_tle = false;
        TLE tle;

    public:
        std::string instrument_name;
        std::string type;

        void set_tle(std::optional<TLE> tle)
        {
            if (tle.has_value())
            {
                _has_tle = true;
                this->tle = tle.value();
            }
        }

        bool has_tle()
        {
            return _has_tle;
        }

        TLE get_tle()
        {
            return tle;
        }

    public:
        virtual void save(std::string directory);
        virtual void load(std::string file);
    };

    std::shared_ptr<Products> loadProducts(std::string path);
}