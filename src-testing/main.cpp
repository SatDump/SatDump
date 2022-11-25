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
    lua.open_libraries(sol::lib::math);

    LuaLogger::bindLogger(lua);

    ////////////////////////////////////////////////////////////////////////////////
    sol::usertype<image::Image<uint16_t>> image16_type = lua.new_usertype<image::Image<uint16_t>>(
        "image16",
        sol::constructors<image::Image<uint16_t>(), image::Image<uint16_t>(size_t, size_t, int)>());

    image16_type["clear"] = &image::Image<uint16_t>::clear;

    image16_type["get"] = [](image::Image<uint16_t> &img, int i)
    { return img[i]; };
    image16_type["set"] = [](image::Image<uint16_t> &img, int i, uint16_t x)
    { img[i] = img.clamp(x); };

    image16_type["depth"] = &image::Image<uint16_t>::depth;
    image16_type["width"] = &image::Image<uint16_t>::width;
    image16_type["height"] = &image::Image<uint16_t>::height;
    image16_type["channels"] = &image::Image<uint16_t>::channels;
    image16_type["size"] = &image::Image<uint16_t>::size;

    image16_type["to_rgb"] = &image::Image<uint16_t>::to_rgb;
    image16_type["to_rgba"] = &image::Image<uint16_t>::to_rgba;

    image16_type["fill_color"] = &image::Image<uint16_t>::fill_color;
    image16_type["fill"] = &image::Image<uint16_t>::fill;
    image16_type["mirror"] = &image::Image<uint16_t>::mirror;
    image16_type["equalize"] = &image::Image<uint16_t>::equalize;
    image16_type["white_balance"] = &image::Image<uint16_t>::white_balance;
    // CROP / CROP-TO
    image16_type["resize"] = &image::Image<uint16_t>::resize;
    image16_type["resize_to"] = &image::Image<uint16_t>::resize_to;
    image16_type["resize_bilinear"] = &image::Image<uint16_t>::resize_bilinear;
    image16_type["brightness_contrast_old"] = &image::Image<uint16_t>::brightness_contrast_old;
    image16_type["linear_invert"] = &image::Image<uint16_t>::linear_invert;
    image16_type["simple_despeckle"] = &image::Image<uint16_t>::simple_despeckle;
    image16_type["median_blur"] = &image::Image<uint16_t>::median_blur;

    image16_type["draw_pixel"] = &image::Image<uint16_t>::draw_pixel;
    image16_type["draw_line"] = &image::Image<uint16_t>::draw_line;
    image16_type["draw_circle"] = &image::Image<uint16_t>::draw_circle;
    image16_type["draw_image"] = &image::Image<uint16_t>::draw_image;
    // image16_type["draw_text"] = &image::Image<uint16_t>::draw_text;

    image16_type["load_png"] = (void(image::Image<uint16_t>::*)(std::string, bool))(&image::Image<uint16_t>::load_png);
    image16_type["save_png"] = &image::Image<uint16_t>::save_png;
    image16_type["load_jpeg"] = (void(image::Image<uint16_t>::*)(std::string))(&image::Image<uint16_t>::load_jpeg);
    image16_type["save_jpeg"] = &image::Image<uint16_t>::save_jpeg;
    image16_type["load_img"] = (void(image::Image<uint16_t>::*)(std::string))(&image::Image<uint16_t>::load_img);
    image16_type["save_img"] = &image::Image<uint16_t>::save_img;
    ////////////////////////////////////////////////////////////////////////////////

    std::ifstream isf(argv[1]);
    std::string lua_src(std::istreambuf_iterator<char>{isf}, {});

    lua.script(lua_src);
}