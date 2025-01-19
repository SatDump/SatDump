#include "lua_bind.h"
#include "products2/product.h"

namespace satdump
{
    namespace lua
    {
        void bind_product(sol::state &lua)
        {
            auto product_type = lua.new_usertype<products::Product>("Product", sol::constructors<products::Product()>());

            product_type["instrument_name"] = &products::Product::instrument_name;
            product_type["type"] = &products::Product::type;
            product_type["set_product_timestamp"] = &products::Product::set_product_timestamp;
            product_type["has_product_timestamp"] = &products::Product::has_product_timestamp;
            product_type["get_product_timestamp"] = &products::Product::get_product_timestamp;
            product_type["set_product_source"] = &products::Product::set_product_source;
            product_type["has_product_source"] = &products::Product::has_product_source;
            product_type["get_product_source"] = &products::Product::get_product_source;
            product_type["save"] = &products::Product::save;
            product_type["load"] = &products::Product::load;

            lua["loadProduct"] = &products::loadProduct;
        }
    }
}