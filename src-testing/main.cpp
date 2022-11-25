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
#include <fstream>
#include "libs/sol2/sol.hpp"
#include "common/image/image.h"

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

    ////////////////////////////////////////////////////////////////////////////////
    sol::usertype<image::Image<uint16_t>> image16_type = lua.new_usertype<image::Image<uint16_t>>(
        "image16",
        sol::constructors<image::Image<uint16_t>(), image::Image<uint16_t>(size_t, size_t, int)>(),
        sol::meta_function::index, [](image::Image<uint16_t> &img, int i)
        { return img[i]; },
        sol::meta_function::new_index, [](image::Image<uint16_t> &img, int i, uint16_t x)
        { return img[i] = x; });

    image16_type["equalize"] = &image::Image<uint16_t>::equalize;
    // image16_type["load_png"] = &image::Image<uint16_t>::load_png;
    image16_type["save_png"] = &image::Image<uint16_t>::save_png;
    ////////////////////////////////////////////////////////////////////////////////

    std::ifstream isf(argv[1]);
    std::string lua_src(std::istreambuf_iterator<char>{isf}, {});

    lua.script(lua_src);
}