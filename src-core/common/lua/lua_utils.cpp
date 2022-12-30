#include "lua_utils.h"

namespace lua_utils
{
    sol::table mapJsonToLua(sol::state &lua, nlohmann::json json)
    {
        sol::table l = lua.create_table();
        for (auto &el : json.items())
        {
            try
            {
                try
                {
                    int index = std::stoi(el.key());
                    if (el.value().is_number_integer())
                        l[index] = el.value().get<int>();
                    else if (el.value().is_number())
                        l[index] = el.value().get<double>();
                    else if (el.value().is_string())
                        l[index] = el.value().get<std::string>();
                    else
                        l[index] = mapJsonToLua(lua, el.value());
                }
                catch (std::exception &e)
                {
                    if (el.value().is_number_integer())
                        l[el.key()] = el.value().get<int>();
                    else if (el.value().is_number())
                        l[el.key()] = el.value().get<double>();
                    else if (el.value().is_string())
                        l[el.key()] = el.value().get<std::string>();
                    else
                        l[el.key()] = mapJsonToLua(lua, el.value());
                }
            }
            catch (std::exception &e)
            {
            }
        }
        return l;
    }
};