#include "lua_utils.h"

#include "common/image/image.h"
#include "common/image/io.h"
#include "common/image/processing.h"
#include "common/image/image_lut.h"
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
        lua.new_usertype<image::Image>(
            name,
            sol::constructors<image::Image(), image::Image(int, size_t, size_t, int)>(),
            "clear", &image::Image::clear,
            "get", [](image::Image& img, size_t i) -> int
            {
                if (i < img.size())
                    return img.get(i);
                else
                    return 0;
            },
            "set", [](image::Image& img, size_t i, int x)
            {
                if (i < img.size())
                    img.set(i, img.clamp(x));
            },
            "depth", &image::Image::depth,
            "width", &image::Image::width,
            "height", &image::Image::height,
            "channels", &image::Image::channels,
            "size", &image::Image::size,
            "to_rgb", &image::Image::to_rgb,
            "to_rgba", &image::Image::to_rgba,
            "fill_color", &image::Image::fill_color,
            "fill", &image::Image::fill,
            "mirror", &image::Image::mirror,
            "resize", &image::Image::resize,
            "resize_to", &image::Image::resize_to,
            "resize_bilinear", &image::Image::resize_bilinear,
            "draw_pixel", &image::Image::draw_pixel,
            "draw_line", &image::Image::draw_line,
            "draw_circle", &image::Image::draw_circle,
            "draw_image", &image::Image::draw_image);

        lua["image_equalize"] = &image::equalize;
        lua["image_white_balance"] = &image::white_balance;
        lua["image_linear_invert"] = &image::linear_invert;
        lua["image_equalize"] = &image::equalize;
        lua["image_simple_despeckle"] = &image::simple_despeckle;
        lua["image_median_blur"] = &image::median_blur;
        lua["image_despeckle"] = &image::kuwahara_filter;

        lua["image_load_png"] = (void (*)(image::Image &, std::string, bool))(&image::load_png);
        lua["image_save_png"] = &image::save_png;
        lua["image_load_jpeg"] = (void (*)(image::Image &, std::string))(&image::load_jpeg);
        lua["image_save_jpeg"] = &image::save_jpeg;
        lua["image_load_img"] = (void (*)(image::Image &, std::string))(&image::load_img);
        lua["image_save_img"] = &image::save_img;
    }

    void bindImageTypes(sol::state &lua)
    {
        bindImageType(lua, "image_t");

        lua["image8_lut_jet"] = &image::LUT_jet<uint8_t>;
        lua["image16_lut_jet"] = &image::LUT_jet<uint16_t>;
    }

    void bindGeoTypes(sol::state &lua)
    {
        lua.new_usertype<geodetic::geodetic_coords_t>("geodetic_coords_t",
            sol::constructors<geodetic::geodetic_coords_t(), geodetic::geodetic_coords_t(double, double, double), geodetic::geodetic_coords_t(double, double, double, bool)>(),
            "toDegs", &geodetic::geodetic_coords_t::toDegs,
            "toRads", &geodetic::geodetic_coords_t::toRads,
            "lat", &geodetic::geodetic_coords_t::lat,
            "lon", &geodetic::geodetic_coords_t::lon,
            "alt", &geodetic::geodetic_coords_t::alt);
    }

    void bindSatProjType(sol::state &lua)
    {
        lua.new_usertype<satdump::SatelliteProjection>("satproj_t",
            "img_size_x", &satdump::SatelliteProjection::img_size_x,
            "img_size_y", &satdump::SatelliteProjection::img_size_y,
            "gcp_spacing_x", &satdump::SatelliteProjection::gcp_spacing_x,
            "gcp_spacing_y", &satdump::SatelliteProjection::gcp_spacing_y,
            "get_position", &satdump::SatelliteProjection::get_position);
    }

    void bindEquProjType(sol::state &lua)
    {
        lua.new_usertype<geodetic::projection::EquirectangularProjection>("EquirectangularProj",
            "init", &geodetic::projection::EquirectangularProjection::init,
            "forward", [](geodetic::projection::EquirectangularProjection& equ, double lon, double lat) -> std::pair<int, int>
            {
                int x2, y2;
                equ.forward(lon, lat, x2, y2);
                return { x2, y2 };
            },
            "reverse", [](geodetic::projection::EquirectangularProjection& equ, int x, int y) -> std::pair<float, float>
            {
                float lon, lat;
                equ.reverse(x, y, lon, lat);
                return { lon, lat };
            });
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