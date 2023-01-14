#pragma once

#include "libs/sol2/sol.hpp"
#include "nlohmann/json.hpp"

namespace lua_utils
{
    sol::table mapJsonToLua(sol::state &lua, nlohmann::json json);

    void bindEquProjType(sol::state &lua);
    void bindSatProjType(sol::state &lua);
    void bindGeoTypes(sol::state &lua);
    void bindImageTypes(sol::state &lua);
};