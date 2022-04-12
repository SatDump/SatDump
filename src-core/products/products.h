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

    class ImageProducts : public Products
    {
    public:
        std::vector<std::tuple<std::string, std::string, image::Image<uint16_t>>> images;
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

        enum Calibration_Type
        {
            POLYNOMIAL,
            POLYNOMIAL_PER_LINE,
            CUSTOM,
        };

        void set_calibration_polynomial(int image_index, std::vector<double> coefficients)
        {
            contents["calibration"][image_index]["type"] = "polynomial";
            contents["calibration"][image_index]["coefs"] = coefficients;
        }

        void set_calibration_polynomial_per_line(int image_index, std::vector<std::vector<double>> coefficients_per_line)
        {
            contents["calibration"][image_index]["type"] = "polynomial_per_line";
            contents["calibration"][image_index]["coefs"] = coefficients_per_line;
        }

        void set_calibration_custom(int image_index, std::string equation)
        {
            contents["calibration"][image_index]["type"] = "custom";
            contents["calibration"][image_index]["equ"] = equation;
        }

    public:
        virtual void save(std::string directory);
        virtual void load(std::string file);
    };

    std::shared_ptr<Products> loadProducts(std::string path);
}