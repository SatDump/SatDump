#pragma once

#include "products.h"
#include "common/image/image.h"

namespace satdump
{
    class ImageProducts : public Products
    {
    public:
        std::vector<std::tuple<std::string, std::string, image::Image<uint16_t>>> images;
        bool has_timestamps = true;
        int bit_depth = 16;

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

        ///////////////////////// Calibration to radiance
        enum Calibration_Type
        {
            POLYNOMIAL,
            CUSTOM,
            POLYNOMIAL_PER_LINE,
            CUSTOM_PER_LINE,
        };

        void set_calibration_polynomial(int image_index, std::vector<double> coefficients)
        {
            contents["calibration"][image_index]["type"] = POLYNOMIAL;
            contents["calibration"][image_index]["coefs"] = coefficients;
        }

        void set_calibration_polynomial_per_line(int image_index, std::vector<std::vector<double>> coefficients_per_line)
        {
            contents["calibration"][image_index]["type"] = POLYNOMIAL_PER_LINE;
            contents["calibration"][image_index]["coefs"] = coefficients_per_line;
        }

        void set_calibration_custom(int image_index, std::string equation)
        {
            contents["calibration"][image_index]["type"] = CUSTOM;
            contents["calibration"][image_index]["equ"] = equation;
        }

        void set_calibration_custom_per_line(int image_index, std::vector<std::string> equation)
        {
            contents["calibration"][image_index]["type"] = CUSTOM_PER_LINE;
            contents["calibration"][image_index]["equ"] = equation;
        }

        bool has_calibation()
        {
            return contents.contains("calibration");
        }

        Calibration_Type get_calibration_type(int image_index)
        {
            return (Calibration_Type)contents["calibration"][image_index]["type"].get<int>();
        }

        void set_wavenumber(int image_index, double waveumber)
        {
            contents["wavenumbers"][image_index] = waveumber;
        }

        double get_wavenumber(int image_index)
        {
            if (contents.contains("wavenumbers"))
                return contents["wavenumbers"][image_index].get<double>();
            else
                return 0;
        }

        std::vector<std::vector<std::vector<double>>> calibration_polynomial_coefs;

        double get_radiance_value(int image_index, int x, int y);

    public:
        virtual void save(std::string directory);
        virtual void load(std::string file);
    };
}