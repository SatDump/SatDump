#pragma once

/**
 * @file satellite_raytracer.h
 */

#include "nlohmann/json.hpp"
#include "common/geodetic/geodetic_coordinates.h"

namespace satdump
{
    namespace proj
    {
        /**
         * @brief Generic base class for satellite raytracers.
         *
         * The general idea is that in most cases in order to geo-reference satellite imagery
         * it is necessary to calculate where the instrument was looking from the attitude &
         * position of the satellite, and raytrace this to Earth. In this instance,
         * it is meant to convert pixel positions in an image to lat/lon.
         *
         * IMPORTANT : The pixel values here MUST already have been ran through a
         * ChannelTransform if required for the channel you're working with!
         *
         * Of course there are many different methods to do this considering how variable
         * the instruments and scanning patterns are, therefore many variants are required.
         *
         * @param d_cfg internal copy of the JSON configuration
         */
        class SatelliteRaytracer
        {
        protected:
            const nlohmann::ordered_json d_cfg;

        public:
            SatelliteRaytracer(nlohmann::ordered_json cfg)
                : d_cfg(cfg)
            {
            }

            /**
             * @brief Get geodetic position of a pixel (can be fractional)
             * @param x pixel position
             * @param y pixel position
             * @param pos output geodetic position
             * @return true on error
             */
            virtual bool get_position(double x, double y, geodetic::geodetic_coords_t &pos) = 0;
        };

        /**
         * @brief Event called to request satellite raytracers
         * @param id
         * @param r vector to return candicates into
         * @param cfg the config to init them with
         */
        struct RequestSatelliteRaytracerEvent
        {
            std::string id;
            std::vector<std::shared_ptr<SatelliteRaytracer>> &r;
            nlohmann::ordered_json cfg;
        };

        /**
         * @brief If supported, get the appropriate satellite raytracer for your configuration
         * @param cfg configuration used for the request
         * @return shared_ptr of the raytracer as base class
         */
        std::shared_ptr<SatelliteRaytracer> get_satellite_raytracer(nlohmann::json cfg);
    }
}