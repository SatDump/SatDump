#pragma once

/**
 * @file projection.h
 */

#include "nlohmann/json.hpp"
#include "common/geodetic/geodetic_coordinates.h"

// Needed for projs. Maybe hide later? TBD TODOREWORK
#include "products2/image/channel_transform.h"
#include "raytrace/satellite_raytracer.h"
#include "common/projection/projs2/proj_json.h"
#include "common/projection/projs/tps_transform.h"

namespace satdump
{
    namespace proj
    {
        /**
         * @brief Extremely generic class to handle projections.
         *
         * This basically abstracts absolutely everything down to a simple
         * class that can be used in 2 lines of codes from a random projection
         * JSON config. By default, all it will do is simply hide the JSON
         * config internally (especially useful to allow manipulating projs
         * in Lua!), but once init() is called, it allows actually performing
         * the projection transform (meant for images).
         *
         * Do note this is not really intended (in most cases) for high-performance
         * scenarios, such as reprojections.
         */
        class Projection
        {
        public:
            enum proj_type_t
            {
                PROJ_INVALID = 0,
                PROJ_STANDARD = 1,
                PROJ_RAYTRACER = 2,
                PROJ_THINPLATESPLINE = 3,
            };

            /**
             * @brief Get forward (Lat/Lon => X/Y) projection type
             * @return projection type
             */
            proj_type_t getFwdType() { return fwd_type; }

            /**
             * @brief Get inverse (X/Y => Lat/Lon) projection type
             * @return projection type
             */
            proj_type_t getInvType() { return inv_type; }

        private:
            nlohmann::json d_cfg;
            bool fwd_valid = false;
            bool rev_valid = false;
            proj_type_t fwd_type = PROJ_INVALID;
            proj_type_t inv_type = PROJ_INVALID;

        private:
            int width = -1;
            int height = -1; // TODOREWORK do we need those?

            std::shared_ptr<SatelliteRaytracer> raytracer;
            ChannelTransform transform;
            ::proj::projection_t std_proj;
            std::shared_ptr<satdump::projection::VizGeorefSpline2D> tps_fwd;
            double tps_fwd_shift_lon = 0, tps_fwd_shift_lat = 0;
            double tps_fwd_xy[2];

        public:
            Projection();
            ~Projection();

            /**
             * @brief Initialize projection, to allow using it to
             * perform the math :-)
             * Disabling doesn't guarantee they won't be initialized
             * internally if required to setup the other!
             * You can use getFwdType/getInvType to check they're
             * not invalid if required.
             * @param fwd init forward proj
             * @param inv init inverse proj
             * @return true if valid
             */
            bool init(bool fwd, bool inv);

            /**
             * @brief Run forward (Lat/Lon => X/Y) projection
             * @param pos output geodetic position
             * @param x pixel position
             * @param y pixel position
             * @return true on error
             */
            bool forward(geodetic::geodetic_coords_t pos, double &x, double &y);

            /**
             * @brief Run forward (X/Y => Lat/Lon) projection
             * @param x pixel position
             * @param y pixel position
             * @param pos output geodetic position
             * @return true on error
             */
            bool inverse(double x, double y, geodetic::geodetic_coords_t &pos);

        public:
            void to_json(nlohmann::json &j) const;
            void from_json(const nlohmann::json &j);

            friend void to_json(nlohmann::json &j, const Projection &v) { v.to_json(j); }
            friend void from_json(const nlohmann::json &j, Projection &v) { v.from_json(j); }
        };
    }
}