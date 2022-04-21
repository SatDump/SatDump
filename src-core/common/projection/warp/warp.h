#pragma once

#include "common/image/image.h"
#include "common/projection/thinplatespline.h"
#ifdef USE_OPENCL
#include <CL/opencl.hpp>
#endif

namespace satdump
{
    namespace warp
    {
        struct WarpOperation
        {
            image::Image<uint16_t> input_image;
            std::vector<projection::GCP> ground_control_points;
            int output_width;
            int output_height;
        };

        struct WarpResult
        {
            image::Image<uint16_t> output_image;

            projection::GCP top_left;
            projection::GCP top_right;
            projection::GCP bottom_right;
            projection::GCP bottom_left;
        };

        projection::VizGeorefSpline2D *initTPSTransform(WarpOperation &op);

        struct WarpCropSettings
        {
            int lat_min;
            int lat_max;
            int lon_min;
            int lon_max;
            int y_min;
            int y_max;
            int x_min;
            int x_max;
        };

        WarpCropSettings choseCropArea(WarpOperation &op);

        WarpResult warpOnCPU(WarpOperation op);

#ifdef USE_OPENCL
        WarpResult warpOnGPU(cl::Context context, cl::Device device, WarpOperation op);
#endif

        WarpResult warpOnAvailable(WarpOperation op);
    }
}