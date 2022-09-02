#include "reprojector.h"
#include "logger.h"
#include "warp/warp.h"
#include "gcp_compute/gcp_compute.h"

#include "sat_proj/sat_proj.h"

#include "projs/equirectangular.h"
#include "projs/stereo.h"

namespace satdump
{
    // Reprojection interface. WIP
    namespace reprojection
    {
        ProjectionResult reproject(ReprojectionOperation &op, float *progress)
        {
            if (progress != nullptr)
                ; // do stuff TODO DELETE

            if (op.use_draw_algorithm) // Use old algorithm, less prone to errors on bad data and faster
            {
                logger->info("Using old algorithm...");

                // Target projs
                geodetic::projection::EquirectangularProjection equi_proj;
                geodetic::projection::StereoProjection stereo_proj;

                // Source projs
                geodetic::projection::EquirectangularProjection equi_proj;
                std::shared_ptr<SatelliteProjection> sat_proj;
            }
            else
            {
                logger->info("Using new algorithm...");

                // Here, we first project to an equirectangular target
                image::Image<uint16_t> warped_image;
                float tl_lon, tl_lat;
                float br_lon, br_lat;

                if (op.source_prj_info["type"] == "equirectangular")
                {
                    // TODO
                }
                else if (op.source_prj_info["type"] == "geos")
                {
                }
                else // Means it's a TPS-handled LEO warp.
                {
                    warp::WarpOperation operation;
                    operation.ground_control_points = satdump::gcp_compute::compute_gcps(op.source_prj_info, op.img_tle, op.img_tim);
                    operation.input_image = op.img;
                    operation.output_width = 2048 * 10;
                    operation.output_height = 1024 * 10;

                    satdump::warp::ImageWarper warper;
                    warper.op = operation;
                    warper.update();

                    satdump::warp::WarpResult result = warper.warp();

                    warped_image = result.output_image;
                    tl_lon = result.top_left.lon;
                    tl_lat = result.top_left.lat;
                    br_lon = result.bottom_right.lon;
                    br_lat = result.bottom_right.lat;
                }
            }
        }
    }
}