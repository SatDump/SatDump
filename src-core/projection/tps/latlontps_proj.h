#pragma once

#include <memory>
#include "projection/utils/tps_transform.h"
#include "common/geodetic/geodetic_coordinates.h"

namespace satdump
{
    namespace proj
    {
        class LatLonTpsProjHelper
        {
        private:
            std::shared_ptr<satdump::projection::VizGeorefSpline2D> tps_fwd;
            std::shared_ptr<satdump::projection::VizGeorefSpline2D> tps_rev;
            double shift_lon = 0, shift_lat = 0;
            double tps_fwd_xy[2];
            double tps_rev_ll[2];

        public:
            LatLonTpsProjHelper(std::vector<satdump::projection::GCP> gcps, bool fwd, bool rev);

            void forward(geodetic::geodetic_coords_t pos, double &x, double &y);
            void reverse(double x, double y, geodetic::geodetic_coords_t &pos);
        };
    }
}