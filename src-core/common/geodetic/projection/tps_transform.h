#pragma once

#include <vector>
#include "thinplatespline.h"

namespace geodetic
{
    namespace projection
    {
        // Ground Control Point
        struct GCP
        {
            double x;
            double y;
            double lon;
            double lat;
        };

        class TPSTransform
        {
        private:
            VizGeorefSpline2D *spline_forward;
            VizGeorefSpline2D *spline_reverse;
            bool fwd_solved;
            bool rev_solved;

            bool has_been_init = false;
            double xy[2] = {0.0, 0.0};

        public:
            int init(std::vector<GCP> gcps);
            TPSTransform();
            TPSTransform(std::vector<GCP> gcps);
            ~TPSTransform();

            void forward(double lon, double lat, double &x, double &y);
            void inverse(double x, double y, double &lon, double &lat);
        };
    };
};