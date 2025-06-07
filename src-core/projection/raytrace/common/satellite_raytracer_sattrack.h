#pragma once

/**
 * @file satellite_raytracer_sattrack.h
 */

#include "common/tracking/tracking.h"
#include "projection/raytrace/satellite_raytracer.h"

namespace satdump
{
    namespace projection
    {
        /**
         * @brief Many of the raytracers require calculating
         * satellite ephemerides. This includes and handles
         * the necessary core for this task.
         *
         * @param sat_tracker satellite tracker (TLE or Ephemeris)
         */
        class SatelliteRaytracerSatTrack : public SatelliteRaytracer
        {
        protected:
            std::shared_ptr<satdump::SatelliteTracker> sat_tracker;

        public:
            SatelliteRaytracerSatTrack(nlohmann::json cfg);
        };
    } // namespace proj
} // namespace satdump