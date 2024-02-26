#pragma once

#include "nlohmann/json.hpp"
#include "common/image/image.h"
#include "products/image_products.h"

namespace satdump
{
    // Reprojection interface. WIP
    namespace reprojection
    {
        // Re-Projection operation
        struct ReprojectionOperation
        {
            image::Image<uint16_t> img;
            int output_width, output_height;
            nlohmann::json target_prj_info;
            bool use_old_algorithm = false;
        };

        image::Image<uint16_t> reproject(ReprojectionOperation &op, float *progress = nullptr);

        std::function<std::pair<int, int>(double, double, int, int)> setupProjectionFunction(int width, int height,
                                                                                             nlohmann::json params,
                                                                                             bool rotate = false);

        struct ProjBounds
        {
            double min_lon;
            double max_lon;
            double min_lat;
            double max_lat;
            bool valid = false;
        };

        ProjBounds determineProjectionBounds(image::Image<uint16_t> &img);
        void tryAutoTuneProjection(ProjBounds bounds, nlohmann::json &params);
    }
}