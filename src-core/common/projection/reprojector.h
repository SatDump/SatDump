#pragma once

#include "nlohmann/json.hpp"

namespace satdump
{
    // Reprojection interface. WIP
    namespace reprojection
    {
        // Projection types
        enum ProjectionType
        {
            PROJ_EQUIRECTANGULAR,
            PROJ_GCPS,
            PROJ_STEREO,
            PROJ_GEOS,
            PROJ_TPERS,
        };

        // Re-Projection operation
        struct ReprojectionOperation
        {
            ProjectionType source_projection;
            nlohmann::json source_prj_info;

            ProjectionType target_projection;
            nlohmann::json target_prj_info;

            bool use_fast_algorithm;
        };
    }
}