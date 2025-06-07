#pragma once

#include "../satellite_raytracer.h"

namespace satdump
{
    namespace projection
    { // TODOREWORK rename?
        class TimestampLineGCPsRaytracer : public SatelliteRaytracer
        {
        private:
            std::vector<double> timestamps;

            struct GCP
            {
                int x;
                int y;
                double lat;
                double lon;
            };
            struct GCPLine
            {
                std::vector<GCP> g;
                double clat;
                double clon;
            };
            std::vector<GCPLine> gcps;

        public:
            TimestampLineGCPsRaytracer(nlohmann::json cfg);
            bool get_position(double x, double y, geodetic::geodetic_coords_t &pos, double *otime);
        };
    }
}