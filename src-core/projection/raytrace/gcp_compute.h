#pragma once

/**
 * @file gcp_compute.h
 */

#include "common/projection/thinplatespline.h"
#include "projection/projection.h"

namespace satdump
{
    namespace proj
    {
        /**
         * @brief Calculate GCPs from a raytracer (or not, as needed) needed to
         * build a Thin Plate Spline transform.
         * @param cfg configuration used for the projection
         * @return vector of GCPs
         */
        std::vector<projection::GCP> compute_gcps(Projection p);
    }
}