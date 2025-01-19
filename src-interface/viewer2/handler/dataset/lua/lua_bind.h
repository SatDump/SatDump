#pragma once

#include "libs/sol2/sol.hpp"

namespace satdump
{
    namespace lua
    {
        void bind_logger(sol::state &lua);

        void bind_image(sol::state &lua);

        void bind_product(sol::state &lua);

        void bind_geodetic(sol::state &lua);

        void bind_projection(sol::state &lua);
    }
}