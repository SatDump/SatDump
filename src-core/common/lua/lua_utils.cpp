#include "lua_utils.h"

#include "common/image/image.h"
#include "common/projection/projs/equirectangular.h"
#include "common/projection/sat_proj/sat_proj.h"
#include "logger.h"

namespace lua_utils
{
    sol::table mapJsonToLua(sol::state &lua, nlohmann::json json)
    {
        sol::table l = lua.create_table();
        char *ptr;
        long index;
        for (auto &el : json.items())
        {
            try
            {
                index = strtol(el.key().c_str(), &ptr, 10);
                if (*ptr == '\0')
                {
                    if (el.value().is_number_integer())
                        l[index] = el.value().get<int>();
                    else if (el.value().is_number())
                        l[index] = el.value().get<double>();
                    else if (el.value().is_string())
                        l[index] = el.value().get<std::string>();
                    else
                        l[index] = mapJsonToLua(lua, el.value());
                }
                else
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
            catch (std::exception &)
            {
            }
        }
        return l;
    }

    void bindImageType(sol::state &lua, std::string name)
    {
        /*
        sol::usertype<image::Image> image_type = lua.new_usertype<image::Image>(
            name,
            sol::constructors<image::Image(), image::Image(int, size_t, size_t, int)>());

        image_type["clear"] = &image::Image::clear;

        image_type["get"] = [](image::Image &img, size_t i) -> int
        {
            if (i < img.size())
                return img.get(i);
            else
                return 0;
        };
        image_type["set"] = [](image::Image &img, size_t i, int x)
        {
            if (i < img.size())
                img.set(i, img.clamp(x));
        };

        image_type["depth"] = &image::Image::depth;
        image_type["width"] = &image::Image::width;
        image_type["height"] = &image::Image::height;
        image_type["channels"] = &image::Image::channels;
        image_type["size"] = &image::Image::size;

        image_type["to_rgb"] = &image::Image::to_rgb;
        image_type["to_rgba"] = &image::Image::to_rgba;

        image_type["fill_color"] = &image::Image::fill_color;
        image_type["fill"] = &image::Image::fill;
        image_type["mirror"] = &image::Image::mirror;
        image_type["equalize"] = &image::Image::equalize;
        image_type["white_balance"] = &image::Image::white_balance;
        // CROP / CROP-TO
        image_type["resize"] = &image::Image::resize;
        image_type["resize_to"] = &image::Image::resize_to;
        image_type["resize_bilinear"] = &image::Image::resize_bilinear;
        image_type["brightness_contrast_old"] = &image::Image::brightness_contrast_old;
        image_type["linear_invert"] = &image::Image::linear_invert;
        image_type["equalize"] = &image::Image::equalize;
        image_type["simple_despeckle"] = &image::Image::simple_despeckle;
        image_type["median_blur"] = &image::Image::median_blur;
        image_type["despeckle"] = &image::Image::kuwahara_filter;

        image_type["draw_pixel"] = &image::Image::draw_pixel;
        image_type["draw_line"] = &image::Image::draw_line;
        image_type["draw_circle"] = &image::Image::draw_circle;
        image_type["draw_image"] = &image::Image::draw_image;
        // image_type["draw_text"] = &image::Image::draw_text;

        image_type["load_png"] = (void(image::Image::*)(std::string, bool))(&image::Image::load_png);
        image_type["save_png"] = &image::Image::save_png;
        image_type["load_jpeg"] = (void(image::Image::*)(std::string))(&image::Image::load_jpeg);
        image_type["save_jpeg"] = &image::Image::save_jpeg;
        image_type["load_img"] = (void(image::Image::*)(std::string))(&image::Image::load_img);
        image_type["save_img"] = &image::Image::save_img; TODOIMG */
    }

    void bindImageTypes(sol::state &lua)
    {
        bindImageType(lua, "image");

        // lua["image8_lut_jet"] = &image::LUT_jet<uint8_t>; TODOIMG
    }

    void bindGeoTypes(sol::state &lua)
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
    }

    void bindSatProjType(sol::state &lua)
    {
        sol::usertype<satdump::SatelliteProjection> type = lua.new_usertype<satdump::SatelliteProjection>("satproj_t");

        type["img_size_x"] = &satdump::SatelliteProjection::img_size_x;
        type["img_size_y"] = &satdump::SatelliteProjection::img_size_y;
        type["gcp_spacing_x"] = &satdump::SatelliteProjection::gcp_spacing_x;
        type["gcp_spacing_y"] = &satdump::SatelliteProjection::gcp_spacing_y;
        type["get_position"] = &satdump::SatelliteProjection::get_position;
    }

    void bindEquProjType(sol::state &lua)
    {
        sol::usertype<geodetic::projection::EquirectangularProjection> type = lua.new_usertype<geodetic::projection::EquirectangularProjection>("EquirectangularProj");

        type["init"] = &geodetic::projection::EquirectangularProjection::init;
        type["forward"] = [](geodetic::projection::EquirectangularProjection &equ, double lon, double lat) -> std::pair<int, int>
        {   int x2, y2;
            equ.forward(lon, lat, x2, y2);
            return {x2, y2}; };
        type["reverse"] = [](geodetic::projection::EquirectangularProjection &equ, int x, int y) -> std::pair<float, float>
        {   float lon, lat;
            equ.reverse(x, y, lon, lat);
            return {lon, lat}; };
    }

    void bindLogger(sol::state &lua)
    {
        lua["ltrace"] = [](std::string log)
        { logger->trace(log); };
        lua["ldebug"] = [](std::string log)
        { logger->debug(log); };
        lua["linfo"] = [](std::string log)
        { logger->info(log); };
        lua["lwarn"] = [](std::string log)
        { logger->warn(log); };
        lua["lerror"] = [](std::string log)
        { logger->error(log); };
        lua["lcritical"] = [](std::string log)
        { logger->critical(log); };
    }
};