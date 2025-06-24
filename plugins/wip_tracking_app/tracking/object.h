#pragma once

#include "common/geodetic/geodetic_coordinates.h"
#include "nlohmann/json.hpp"
#include "tracking.h"
#include <map>
#include <memory>

namespace satdump
{
    namespace tracking
    {
        class ObjectBackend
        {
        };

        class Object
        {
        protected:
            std::map<backend_type_t, std::shared_ptr<ObjectBackend>> backends;

        public:
            Object(nlohmann::json p);
            ~Object();

            virtual geodetic::geodetic_coords_t getGeoPos(double time);
        };
    } // namespace tracking
} // namespace satdump