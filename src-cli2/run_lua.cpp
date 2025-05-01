#include "run_lua.h"

#include "common/utils.h"
#include "handler/dataset/lua/lua_bind.h"
#include "libs/sol2/sol.hpp"
#include "logger.h"

namespace satdump
{
    int runLua(std::string file)
    {
        std::string lua_code = loadFileToString(file);

        try
        {
            sol::state lua;

            lua.open_libraries(sol::lib::base);
            lua.open_libraries(sol::lib::string);
            lua.open_libraries(sol::lib::math);
            lua.open_libraries(sol::lib::table);
            lua.open_libraries(sol::lib::io);

            lua::bind_logger(lua);
            lua::bind_image(lua);
            lua::bind_product(lua);
            lua::bind_geodetic(lua);
            lua::bind_projection(lua);
            lua::bind_handler_image(lua);

            // Run
            lua.script(lua_code);
        }
        catch (sol::error &e)
        {
            logger->error(e.what());
            return 1;
        }

        return 0;
    }
} // namespace satdump