#pragma once

#include "common/geodetic/geodetic_coordinates.h"
#include "tracking.h"
#include <map>
#include <memory>

namespace satdump
{
    namespace tracking
    {
        class StationBackend
        {
        };

        class Station
        {
        protected:
            std::map<backend_type_t, std::shared_ptr<StationBackend>> backends;

        protected:
            const std::string name;

        public:
            Station(std::string name) : name(name) {}
            ~Station() {}

            void setPosition(geodetic::geodetic_coords_t coords);
            geodetic::geodetic_coords_t getPosition();
        };
    } // namespace tracking
} // namespace satdump