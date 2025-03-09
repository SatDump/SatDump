#include "variable_manager.h"
#include "imgui/imgui.h"
#include "imgui/imgui_stdlib.h"
#include "common/widgets/json_editor.h"
#include "libs/sol2/sol.hpp"
#include "logger.h"

namespace satdump
{
    namespace ndsp
    {
        void LuaVariableManager::drawVariables()
        {
            widgets::JSONTableEditor(variables, "Variables");
        }

        template <typename T>
        T LuaVariableManager::getVariable(std::string var_str)
        {
            sol::state lua;
            lua.open_libraries(sol::lib::string);
            lua.open_libraries(sol::lib::math);

            for (auto &j : variables.items())
            {
                if (j.value().is_number_integer())
                    lua[j.key()] = j.value().get<int>();
                else if (j.value().is_number())
                    lua[j.key()] = j.value().get<double>();
                else
                    logger->error("Type unsupported for Lua! : " + j.key());
            }

            lua.script("output = " + var_str);

            return lua["output"].get<T>();
        }

        double LuaVariableManager::getDoubleVariable(std::string var_str) { return getVariable<double>(var_str); }
        int LuaVariableManager::getIntVariable(std::string var_str) { return getVariable<int>(var_str); }
    }
}