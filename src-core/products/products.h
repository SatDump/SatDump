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
    protected:
        nlohmann::json contents;

    public:
        std::string instrument_name;

    public:
        virtual void save(std::string directory);
    };

    class ImageProducts : public Products
    {
    public:
        std::vector<std::pair<std::string, image::Image<uint16_t>>> images;
        bool has_timestamps = true;

        enum Timestamp_Type
        {
            TIMESTAMP_LINE,
            TIMESTAMP_MULTIPLE_LINES,
            TIMESTAMP_IFOV,
        };

        Timestamp_Type timestamp_type;

        void set_timestamps(std::vector<double> timestamps)
        {
            contents["timestamps"] = timestamps;
        }

    public:
        virtual void save(std::string directory);
    };
}