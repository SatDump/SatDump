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
#include "init.h"
#include "products2/image_product.h"
#include "common/image/io.h"
#include "common/image/processing.h"

#include "products2/image/product_equation.h"
#include "products2/image/image_calibrator.h"

#include "common/utils.h"

#include "libs/sol2/sol.hpp"

#include "common/projection/reprojector.h"

using namespace satdump;

int main(int argc, char *argv[])
{
    initLogger();

    logger->set_level(slog::LOG_OFF);
    satdump::initSatdump();
    completeLoggerInit();
    logger->set_level(slog::LOG_TRACE);

    try
    {
        sol::state lua;

        lua.open_libraries(sol::lib::base);
        lua.open_libraries(sol::lib::string);
        lua.open_libraries(sol::lib::math);

        // Product
        auto product_type = lua.new_usertype<products::Product>("Product", sol::constructors<std::shared_ptr<products::Product>()>());
        product_type["instrument_name"] = &products::Product::instrument_name;
        product_type["type"] = &products::Product::type;
        product_type["set_product_timestamp"] = &products::Product::set_product_timestamp;
        product_type["has_product_timestamp"] = &products::Product::has_product_timestamp;
        product_type["get_product_timestamp"] = &products::Product::get_product_timestamp;
        product_type["set_product_source"] = &products::Product::set_product_source;
        product_type["has_product_source"] = &products::Product::has_product_source;
        product_type["save"] = &products::Product::save;
        product_type["load"] = &products::Product::load;

        //     lua["loadProduct"] = sol::overload( &products::loadProduct, [](products::ImageProduct*p, std::string equ) { &products::loadProduct} ); ;

        // ImageProduct
        auto image_product_type = lua.new_usertype<products::ImageProduct>("ImageProduct", sol::constructors<std::shared_ptr<products::ImageProduct>()>(), sol::base_classes, sol::bases<products::Product>());

        lua["generate_equation_product_composite"] = sol::overload(
            // (image::Image(*)(products::ImageProduct *, std::string))(&products::generate_equation_product_composite)
            [](products::ImageProduct *p, std::string c)
            { return products::generate_equation_product_composite(p, c); });
        lua["generate_calibrated_product_channel"] = sol::overload(
            // (image::Image(*)(products::ImageProduct *, std::string, double, double))(&products::generate_calibrated_product_channel)
            [](products::ImageProduct *p, std::string c, double a, double b)
            { return products::generate_calibrated_product_channel(p, c, a, b); },
            [](products::ImageProduct *p, std::string c, double a, double b, std::string u)
            { return products::generate_calibrated_product_channel(p, c, a, b, u); });

        // Image
        sol::usertype<image::Image> image_type = lua.new_usertype<image::Image>("Image", sol::constructors<image::Image(), image::Image(int, size_t, size_t, int)>());

        image_type["depth"] = &image::Image::depth;
        image_type["width"] = &image::Image::width;
        image_type["height"] = &image::Image::height;
        image_type["channels"] = &image::Image::channels;
        image_type["size"] = &image::Image::size;

        image_type["draw_image"] = &image::Image::draw_image;

        lua["image_load_img"] = (void (*)(image::Image &, std::string))(&image::load_img);
        lua["image_save_img"] = (void (*)(image::Image &, std::string))(&image::save_img);

        // Run
        lua.script_file("../src-testing/test.lua");

        image::Image img = lua["final_img"];

        satdump::reprojection::ReprojectionOperation op;
        op.img = &img;
        op.output_width = 2048 * 4;
        op.output_height = 1024 * 4;
        nlohmann::json cfg;
        double projections_equirectangular_tl_lon = -180;
        double projections_equirectangular_tl_lat = 90;
        double projections_equirectangular_br_lon = 180;
        double projections_equirectangular_br_lat = -90;
        cfg["type"] = "equirec";
        cfg["offset_x"] = projections_equirectangular_tl_lon;
        cfg["offset_y"] = projections_equirectangular_tl_lat;
        cfg["scalar_x"] = (projections_equirectangular_br_lon - projections_equirectangular_tl_lon) / double(op.output_width);
        cfg["scalar_y"] = (projections_equirectangular_br_lat - projections_equirectangular_tl_lat) / double(op.output_height);
        cfg["scale_x"] = 0.016;
        cfg["scale_y"] = 0.016;
        op.target_prj_info = cfg;
        auto img2 = reprojection::reproject(op);
        image::save_img(img2, "test_lua2.png");
    }
    catch (sol::error &e)
    {
        logger->error(e.what());
    }
}
