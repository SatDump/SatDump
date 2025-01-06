#include "dataset_product_handler.h"
#include "logger.h"

// TODOREWORK
#include "common/projection/warp/warp.h"
#include "common/projection/gcp_compute/gcp_compute.h"
#include "common/projection/projs2/proj_json.h"

#include "libs/sol2/sol.hpp"

//
#include "products2/image/product_equation.h"
#include "products2/image/image_calibrator.h"
#include "products2/image_product.h"
#include "common/image/io.h"
#include "common/projection/reprojector.h"

#include "../image/image_handler.h"

SOL_BASE_CLASSES(satdump::viewer::ImageHandler, satdump::viewer::Handler);
SOL_DERIVED_CLASSES(satdump::viewer::Handler, satdump::viewer::ImageHandler);

// SOL_BASE_CLASSES(satdump::products::ImageProduct, satdump::products::Product);
// SOL_DERIVED_CLASSES(satdump::products::Product, satdump::products::ImageProduct);

namespace satdump
{
    namespace viewer
    {
        void DatasetProductHandler::run_lua()
        {
            std::string lua_code = editor.GetText();

            try
            {
                sol::state lua;

                lua.open_libraries(sol::lib::base);
                lua.open_libraries(sol::lib::string);
                lua.open_libraries(sol::lib::math);

                // Product
                auto product_type = lua.new_usertype<products::Product>("Product", sol::constructors<products::Product()>());
                product_type["instrument_name"] = &products::Product::instrument_name;
                product_type["type"] = &products::Product::type;
                product_type["set_product_timestamp"] = &products::Product::set_product_timestamp;
                product_type["has_product_timestamp"] = &products::Product::has_product_timestamp;
                product_type["get_product_timestamp"] = &products::Product::get_product_timestamp;
                product_type["set_product_source"] = &products::Product::set_product_source;
                product_type["has_product_source"] = &products::Product::has_product_source;
                product_type["save"] = &products::Product::save;
                product_type["load"] = &products::Product::load;

                lua["loadProduct"] = &products::loadProduct;

                // ImageProduct
                auto image_product_type = lua.new_usertype<products::ImageProduct>("ImageProduct", sol::constructors<products::ImageProduct()>(), sol::base_classes, sol::bases<products::Product>());

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

                // ImageHandler
                sol::usertype<ImageHandler> image_handler_type = lua.new_usertype<ImageHandler>("ImageHandler",
                                                                                                // sol::constructors<std::shared_ptr<ImageHandler>(), std::shared_ptr<ImageHandler>(image::Image)>(),
                                                                                                sol::call_constructor,
                                                                                                sol::factories([](image::Image img)
                                                                                                               { return std::make_shared<ImageHandler>(img); },
                                                                                                               [](std::shared_ptr<Handler> h)
                                                                                                               { return std::dynamic_pointer_cast<ImageHandler>(h); }),
                                                                                                sol::base_classes, sol::bases<Handler>());
                // image_type["depth"] = &image::Image::depth;

                // Experimental
                lua["add_handler_to_products"] = [this](std::shared_ptr<Handler> p) // TODO, find why THIS specifically crashes...
                {
                    if (p)
                        addSubHandler(p);
                };

                lua["warp_image_test"] = [](image::Image img)
                {
                    auto prj_cfg = image::get_metadata_proj_cfg(img);
                    warp::WarpOperation operation;
                    operation.ground_control_points = satdump::gcp_compute::compute_gcps(prj_cfg, img.width(), img.height());
                    operation.input_image = &img;
                    operation.output_rgba = true;
                    // TODO : CHANGE!!!!!!
                    int l_width = prj_cfg.contains("f_width") ? prj_cfg["f_width"].get<int>() : std::max<int>(img.width(), 512) * 10;
                    operation.output_width = l_width;
                    operation.output_height = l_width / 2;

                    logger->trace("Warping size %dx%d", l_width, l_width / 2);

                    satdump::warp::WarpResult result = satdump::warp::performSmartWarp(operation);

                    auto src_proj = proj::projection_t();
                    src_proj.type = proj::ProjType_Equirectangular;
                    src_proj.proj_offset_x = result.top_left.lon;
                    src_proj.proj_offset_y = result.top_left.lat;
                    src_proj.proj_scalar_x = (result.bottom_right.lon - result.top_left.lon) / double(result.output_image.width());
                    src_proj.proj_scalar_y = (result.bottom_right.lat - result.top_left.lat) / double(result.output_image.height());

                    image::set_metadata_proj_cfg(result.output_image, src_proj);

                    return result.output_image;
                };

                lua["get_instrument_products"] = [this](std::string v, int index)
                {
                    std::vector<products::Product *> pro;
                    for (auto &h : dataset_handler->all_products)
                        if (h->instrument_name == v)
                            pro.push_back(h.get());
                    return index < pro.size() ? pro[index] : nullptr;
                };

                // Run
                lua.script(lua_code);

                /*
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
                                image::save_img(img2, "test_lua2.png");*/
            }
            catch (sol::error &e)
            {
                logger->error(e.what());
            }
        }
    }
}
