/**********************************************************************
 * This file is used for testing random stuff without running the
 * whole of SatDump, which comes in handy for debugging individual
 * elements before putting them all together in modules...
 *
 * If you are an user, ignore this file which will not be built by
 * default, and if you're a developper in need of doing stuff here...
 * Go ahead!
 *
 * Don't judge the code you might see in there! :)
 **********************************************************************/

#include "logger.h"

#include "libs/sol2/sol.hpp"

class LuaLogger
{
public:
    LuaLogger() {}

    void trace(std::string log) { logger->trace(log); }
    void debug(std::string log) { logger->debug(log); }
    void info(std::string log) { logger->info(log); }
    void warn(std::string log) { logger->warn(log); }
    void error(std::string log) { logger->error(log); }
    void critical(std::string log) { logger->critical(log); }

    static void bindLogger(sol::state &lua)
    {
        lua.new_usertype<LuaLogger>("lua_logger",
                                    "trace", &LuaLogger::trace,
                                    "debug", &LuaLogger::debug,
                                    "info", &LuaLogger::info,
                                    "warn", &LuaLogger::warn,
                                    "error", &LuaLogger::error,
                                    "critical", &LuaLogger::critical);
        lua["logger"] = std::make_shared<LuaLogger>();
    }
};

int main(int /*argc*/, char *argv[])
{
    initLogger();

    sol::state lua;

    lua.open_libraries(sol::lib::base);
    lua.open_libraries(sol::lib::string);

    LuaLogger::bindLogger(lua);

    lua.script(
        "for i = 0, 10, 1 \n"
        "do \n"
        "logger:warn(string.format('Test %d', i)) \n"
        "end \n");
}