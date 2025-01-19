#include "lua_bind.h"
#include "projection/projection.h"

namespace satdump
{
    namespace lua
    {
        void bind_projection(sol::state &lua)
        {
            auto product_type = lua.new_usertype<proj::Projection>("Projection", sol::constructors<proj::Projection()>());

            product_type["init"] = &proj::Projection::init;
            // TODOREWORK            product_type["forward"] = &proj::Projection::forward;
            // TODOREWORK            product_type["inverse"] = &proj::Projection::inverse;
        }
    }
}