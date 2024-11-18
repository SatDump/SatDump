#pragma once

#include "common/image/image.h"
#include "common/projection/thinplatespline.h"
#include <memory>
#include <functional>

namespace satdump
{
    namespace warp
    {
        /*
        Image warping operation, taking an image and Ground Control Points.
        output_width and output_height do NOT define the *actual* output
        height / width. It defines the overall resolution assuming it was
        covering 360 degrees of longitude and 180 of latitude.
        */
        struct WarpOperation
        {
            image::Image *input_image;
            std::vector<projection::GCP> ground_control_points;
            int output_width;
            int output_height;
            int output_rgba = false;

            int shift_lon = 0;
            int shift_lat = 0;
        };

        /*
        Result of an image warping operation, holding the image and resulting
        4 GCPs.
        Those GCPs allow referencing the resulting image to Lat / Lon using a
        much less compute-intensive affine transform.
        */
        struct WarpResult
        {
            image::Image output_image;
            projection::GCP top_left;
            projection::GCP top_right;
            projection::GCP bottom_right;
            projection::GCP bottom_left;
        };

        WarpResult performSmartWarp(WarpOperation op, float *progress = nullptr);
    }
}