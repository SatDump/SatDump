#include "image_products.h"
#include "logger.h"

namespace satdump
{
    void ImageProducts::save(std::string directory)
    {
        type = "image";
        contents["has_timestamps"] = has_timestamps;
        contents["bit_depth"] = bit_depth;

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

        if (has_calibation())
            calibration_polynomial_coefs.resize(contents["images"].size());

        has_timestamps = contents["has_timestamps"].get<bool>();
        bit_depth = contents["bit_depth"].get<int>();

        for (size_t c = 0; c < contents["images"].size(); c++)
        {
            image::Image<uint16_t> image;
            logger->info("Loading " + contents["images"][c]["file"].get<std::string>());
            image.load_png(directory + "/" + contents["images"][c]["file"].get<std::string>());
            images.push_back({contents["images"][c]["file"].get<std::string>(), contents["images"][c]["name"].get<std::string>(), image});

            // Also load calibration if present
            if (has_calibation())
            {
                if (get_calibration_type(c) == POLYNOMIAL)
                    calibration_polynomial_coefs[c][0] = contents["calibration"][c]["coefs"].get<std::vector<double>>();
                else if (get_calibration_type(c) == POLYNOMIAL_PER_LINE)
                    calibration_polynomial_coefs[c] = contents["calibration"][c]["coefs"].get<std::vector<std::vector<double>>>();
                // CUSTOM NOT IMPLEMENTED YET
            }
        }
    }

    double ImageProducts::get_radiance_value(int image_index, int x, int y)
    {
        std::vector<double> coefs;

        if (has_calibation())
        {
            if (get_calibration_type(image_index) == POLYNOMIAL)
                coefs = calibration_polynomial_coefs[image_index][0];
            else if (get_calibration_type(image_index) == POLYNOMIAL_PER_LINE)
                coefs = calibration_polynomial_coefs[image_index][y];
            else
                return 0;
        }
        else
            return 0;

        double raw_value = std::get<2>(images[image_index])[y * std::get<2>(images[image_index]).width() + x] >> (16 - bit_depth);

        double radiance = 0;
        int level = 0;
        for (double c : coefs)
            radiance += c * powf(raw_value, level++);

        return radiance;
    }
}