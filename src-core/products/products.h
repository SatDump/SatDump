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

        void set_calibration_polynomial(std::vector<float> coefficients)
        {
            contents["calibration"]["type"] = "polynomial";
            contents["calibration"]["coefs"] = coefficients;
        }

        void set_calibration_polynomial_per_line(std::vector<std::vector<float>> coefficients_per_line)
        {
            contents["calibration"]["type"] = "polynomial_per_line";
            contents["calibration"]["coefs"] = coefficients_per_line;
        }

        void set_calibration_custom(std::string equation)
        {
            contents["calibration"]["type"] = "custom";
            contents["calibration"]["equ"] = equation;
        }

    public:
        virtual void save(std::string directory);
        virtual void load(std::string file);
    };
}