#pragma once
#define USE_OPENCL
#include "common/image/image.h"
#include "common/projection/thinplatespline.h"
#ifdef USE_OPENCL
#if 0
#define CL_HPP_TARGET_OPENCL_VERSION 120
#define CL_HPP_MINIMUM_OPENCL_VERSION 120
#endif
#define __CL_ENABLE_EXCEPTIONS
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

        // Should we even expose those 2 functions?
        WarpResult warpOnCPU(WarpOperation op);

#ifdef USE_OPENCL
        WarpResult warpOnGPU_fp64(cl::Context context, cl::Device device, WarpOperation op);
        WarpResult warpOnGPU_fp32(cl::Context context, cl::Device device, WarpOperation op);
#endif

        WarpResult warpOnAvailable(WarpOperation op);
    }
}