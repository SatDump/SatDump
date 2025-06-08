#include "reproject_img.h"
#include "image/meta.h"
#include "projection/projection.h"
#include "projection/reprojector.h"

namespace satdump
{
    namespace projection
    {
        image::Image reprojectImage(image::Image &input, nlohmann::json target_prj, float *progress)
        {
            Projection proj = target_prj;

            // TODOREWORK!!!!
            ReprojectionOperation op;
            op.img = &input;
            op.output_width = proj.width;
            op.output_height = proj.height;
            op.target_prj_info = proj;
            //            op.target_prj_info["width"] = proj->width;
            //            op.target_prj_info["height"] = proj->height;
            auto cfg = image::get_metadata_proj_cfg(*op.img);
            cfg["width"] = input.width();
            cfg["height"] = input.height();
            image::set_metadata_proj_cfg(*op.img, cfg);
            return reproject(op, progress);
        }
    } // namespace proj
} // namespace satdump