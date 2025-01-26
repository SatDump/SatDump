#include "lua_bind.h"
#include "common/geodetic/geodetic_coordinates.h"

namespace satdump
{
    namespace lua
    {
        void bind_geodetic(sol::state &lua)
        {
            sol::usertype<geodetic::geodetic_coords_t> type = lua.new_usertype<geodetic::geodetic_coords_t>("geodetic_coords_t",
                                                                                                            sol::constructors<
                                                                                                                geodetic::geodetic_coords_t(),
                                                                                                                geodetic::geodetic_coords_t(double, double, double),
                                                                                                                geodetic::geodetic_coords_t(double, double, double, bool)>());

            type["toDegs"] = &geodetic::geodetic_coords_t::toDegs;
            type["toRads"] = &geodetic::geodetic_coords_t::toRads;
            type["lat"] = &geodetic::geodetic_coords_t::lat;
            type["lon"] = &geodetic::geodetic_coords_t::lon;
            type["alt"] = &geodetic::geodetic_coords_t::alt;

            // TODOREWORK VINCENTYS
        }
    }
}