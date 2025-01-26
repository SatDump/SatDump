#include "lua_bind.h"
#include "products2/image_product.h"
#include "products2/image/image_calibrator.h"
#include "products2/image/product_equation.h"

namespace satdump
{
    namespace lua
    {
        void bind_product_image(sol::state &lua)
        {
            auto image_product_type = lua.new_usertype<products::ImageProduct>("ImageProduct", sol::constructors<products::ImageProduct()>(), sol::base_classes, sol::bases<products::Product>());

            lua["generate_equation_product_composite"] = sol::overload(
                // (image::Image(*)(products::ImageProduct *, std::string))(&products::generate_equation_product_composite)
                [](products::Product *p, std::string c)
                {
                    if (!p)
                        throw sol::error("Invalid pointer!");
                    if (p->type != "image")
                        throw sol::error("Invalid product type!");
                    return products::generate_equation_product_composite((products::ImageProduct *)p, c);
                });
            lua["generate_calibrated_product_channel"] = sol::overload(
                // (image::Image(*)(products::ImageProduct *, std::string, double, double))(&products::generate_calibrated_product_channel)
                [](products::ImageProduct *p, std::string c, double a, double b)
                { return products::generate_calibrated_product_channel(p, c, a, b); },
                [](products::ImageProduct *p, std::string c, double a, double b, std::string u)
                { return products::generate_calibrated_product_channel(p, c, a, b, u); });
        }
    }
}