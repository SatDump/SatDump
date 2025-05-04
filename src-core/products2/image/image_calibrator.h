#pragma once

#include "../image_product.h" // TODOREWORK remove?
#include "calibration_units.h"
#include "nlohmann/json.hpp"

namespace satdump
{
    namespace products
    {
        class ImageCalibrator
        {
        protected:
            ImageProduct *d_pro;
            nlohmann::json d_cfg;

        private:
            int depth_diff = 0;
            uint32_t val = 0;

        public:
            ImageCalibrator(ImageProduct *p, nlohmann::json c) : d_pro(p), d_cfg(c) {}

            inline double compute(int idx, int x, int y)
            {
                auto &i = d_pro->images[idx];
                if (0 <= x && x < i.image.width() && 0 <= y && y < i.image.height())
                {
                    val = d_pro->get_raw_channel_val(idx, x, y);
                    return compute(i.abs_index, x, y, val);
                }
                else
                    return CALIBRATION_INVALID_VALUE;
            }

        protected:
            virtual double compute(int abs_idx, int x, int y, uint32_t val) = 0;
        };

        struct RequestImageCalibratorEvent
        {
            std::string id;
            std::vector<std::shared_ptr<ImageCalibrator>> &calibrators;
            ImageProduct *products;
            nlohmann::json calib;
        };

        std::shared_ptr<ImageCalibrator> get_calibrator_from_product(ImageProduct *p);
        image::Image generate_calibrated_product_channel(ImageProduct *product, std::string channel_name, double range_min, double range_max, std::string output_unit = "", float *progess = nullptr);
    } // namespace products
} // namespace satdump