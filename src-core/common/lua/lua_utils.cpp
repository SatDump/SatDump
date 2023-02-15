#include "lua_utils.h"

#include "common/image/image.h"
#include "common/projection/projs/equirectangular.h"
#include "common/projection/sat_proj/sat_proj.h"

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

    template <typename T>
    void bindImageType(sol::state &lua, std::string name)
    {
        sol::usertype<image::Image<T>> image_type = lua.new_usertype<image::Image<T>>(
            name,
            sol::constructors<image::Image<T>(), image::Image<uint16_t>(size_t, size_t, int)>());

        image_type["clear"] = &image::Image<T>::clear;

        image_type["get"] = [](image::Image<T> &img, int i)
        { return img[i]; };
        image_type["set"] = [](image::Image<T> &img, int i, T x)
        { img[i] = img.clamp(x); };

        image_type["depth"] = &image::Image<T>::depth;
        image_type["width"] = &image::Image<T>::width;
        image_type["height"] = &image::Image<T>::height;
        image_type["channels"] = &image::Image<T>::channels;
        image_type["size"] = &image::Image<T>::size;

        image_type["to_rgb"] = &image::Image<T>::to_rgb;
        image_type["to_rgba"] = &image::Image<T>::to_rgba;

        image_type["fill_color"] = &image::Image<T>::fill_color;
        image_type["fill"] = &image::Image<T>::fill;
        image_type["mirror"] = &image::Image<T>::mirror;
        image_type["equalize"] = &image::Image<T>::equalize;
        image_type["white_balance"] = &image::Image<T>::white_balance;
        // CROP / CROP-TO
        image_type["resize"] = &image::Image<T>::resize;
        image_type["resize_to"] = &image::Image<T>::resize_to;
        image_type["resize_bilinear"] = &image::Image<T>::resize_bilinear;
        image_type["brightness_contrast_old"] = &image::Image<T>::brightness_contrast_old;
        image_type["linear_invert"] = &image::Image<T>::linear_invert;
        image_type["simple_despeckle"] = &image::Image<T>::simple_despeckle;
        image_type["median_blur"] = &image::Image<T>::median_blur;

        image_type["draw_pixel"] = &image::Image<T>::draw_pixel;
        image_type["draw_line"] = &image::Image<T>::draw_line;
        image_type["draw_circle"] = &image::Image<T>::draw_circle;
        image_type["draw_image"] = &image::Image<T>::draw_image;
        // image_type["draw_text"] = &image::Image<T>::draw_text;

        image_type["load_png"] = (void(image::Image<T>::*)(std::string, bool))(&image::Image<T>::load_png);
        image_type["save_png"] = &image::Image<T>::save_png;
        image_type["load_jpeg"] = (void(image::Image<T>::*)(std::string))(&image::Image<T>::load_jpeg);
        image_type["save_jpeg"] = &image::Image<T>::save_jpeg;
        image_type["load_img"] = (void(image::Image<T>::*)(std::string))(&image::Image<T>::load_img);
        image_type["save_img"] = &image::Image<T>::save_img;
    }

    void bindImageTypes(sol::state &lua)
    {
        bindImageType<uint8_t>(lua, "image8");
        bindImageType<uint16_t>(lua, "image16");
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
};