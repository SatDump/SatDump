#include "variable_manager.h"
#include "imgui/imgui.h"
#include "imgui/imgui_stdlib.h"
#include "common/widgets/json_editor.h"
#include "libs/sol2/sol.hpp"
#include "logger.h"
#include "core/exception.h"

#include <typeinfo>

namespace satdump
{
    namespace ndsp
    {
        void LuaVariableManager::drawVariables()
        {
            widgets::JSONTableEditor(variables, "Variables");
        }

        nlohmann::json LuaVariableManager::getVariable(std::string var_str)
        {
            sol::state lua;
            lua.open_libraries(sol::lib::string);
            lua.open_libraries(sol::lib::math);

            for (auto &j : variables.items())
            {
                if (j.value().is_number())
                {
                    if (j.value().is_number_integer())
                        lua[j.key()] = j.value().get<int64_t>();
                    else
                        lua[j.key()] = j.value().get<double>();
                }
                else if (j.value().is_string())
                    lua[j.key()] = j.value().get<std::string>();
                else
                    logger->error("Type unsupported for Lua! : " + j.key());
            }

            lua.script("output = " + var_str);

            printf("%s\n", sol::type_name(lua, lua["output"].get_type()).c_str());

            sol::object o = lua["output"];

            if (o.is<bool>())
                return o.as<bool>();
            else if (o.is<int64_t>())
                return o.as<int64_t>();
            else if (o.is<std::string>())
                return o.as<std::string>();
            else if (o.is<double>())
                return o.as<double>();
            else
            {
                lua.open_libraries(sol::lib::base);
                lua.script("print(output)");
                throw satdump_exception("Could not convert type from Lua to JSON");
            }
        }
    }
}