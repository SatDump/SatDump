#include "lua_bind.h"
#include "projection/projection.h"
#include "projection/reprojector.h"

#include "common/image/meta.h"

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

            lua["image_reproject"] = [](image::Image img, proj::Projection proj, int width, int height)
            {
                // TODOREWORK!!!!
                proj::ReprojectionOperation op;
                op.img = &img;
                op.output_width = width;
                op.output_height = height;
                op.target_prj_info = proj;
                op.target_prj_info["width"] = width;
                op.target_prj_info["height"] = height;
                auto cfg = image::get_metadata_proj_cfg(*op.img);
                cfg["width"] = img.width();
                cfg["height"] = img.height();
                image::set_metadata_proj_cfg(*op.img, cfg);
                return proj::reproject(op);
            };
        }
    }
}