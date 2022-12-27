#pragma once

#include "libs/sol2/sol.hpp"
#include "nlohmann/json.hpp"

namespace lua_utils
{
    sol::table mapJsonToLua(sol::state &lua, nlohmann::json json);
};