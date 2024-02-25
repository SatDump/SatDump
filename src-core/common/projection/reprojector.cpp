#include "reprojector.h"
#include "logger.h"
#include "warp/warp.h"
#include "gcp_compute/gcp_compute.h"

#include "sat_proj/sat_proj.h"

// #include "projs/equirectangular.h"
// #include "projs/mercator.h"
// #include "projs/stereo.h"
// #include "projs/tpers.h"
// #include "projs/azimuthal_equidistant.h"

// #include "sat_proj/geo_projection.h"
#include "projs/tps_transform.h"
// #include "reproj/reproj.h"

#include "common/projection/projs2/proj_json.h"
#include "common/image/image_meta.h"

namespace satdump
{
    namespace reprojection
    {
        inline void transposePixel(image::Image<uint16_t> &in, image::Image<uint16_t> &out, int ix, int iy, int ox, int oy)
        {
            if (ix >= (int)in.width() || iy >= (int)in.height() || ix < 0 || iy < 0)
                return;
            if (ox >= (int)out.width() || oy >= (int)out.height() || ox < 0 || oy < 0)
                return;

            if (in.channels() == 4)
            {
                for (int c = 0; c < in.channels(); c++)
                    out.channel(c)[oy * out.width() + ox] = in.channel(c)[iy * in.width() + ix];
            }
            else if (in.channels() == 3)
            {
                for (int c = 0; c < in.channels(); c++)
                    out.channel(c)[oy * out.width() + ox] = c == 3 ? 65535 : in.channel(c)[iy * in.width() + ix];
                if (out.channels() == 4)
                    out.channel(3)[oy * out.width() + ox] = 65535;
            }
            else
            {
                for (int c = 0; c < in.channels(); c++)
                    out.channel(c)[oy * out.width() + ox] = c == 3 ? 65535 : in.channel(0)[iy * in.width() + ix];
                if (out.channels() == 4)
                    out.channel(3)[oy * out.width() + ox] = 65535;
            }
        }

        ProjectionResult reproject(ReprojectionOperation &op, float *progress)
        {
            ProjectionResult result_prj;

            if (op.img.size() == 0)
                throw std::runtime_error("Can't reproject an empty image!");

            result_prj.img.init(op.output_width, op.output_height, 4);
            result_prj.settings = op.target_prj_info;

            // Attempt to init target proj
            proj::projection_t trg_proj;
            bool trg_proj_err = false;
            try
            {
                trg_proj = op.target_prj_info;
            }
            catch (std::exception &e)
            {
                trg_proj_err = true;
            }

            if (proj::projection_setup(&trg_proj) || trg_proj_err)
            {
                logger->error("Invalid target projection? Could not init! %d : \n%s", (int)trg_proj_err, op.target_prj_info.dump(4).c_str());
                return result_prj;
            }

            // Attempt to init source proj
            proj::projection_t src_proj;
            bool src_proj_err = false;
            try
            {
                src_proj = op.source_prj_info;
            }
            catch (std::exception &e)
            {
                src_proj_err = true;
            }

            if (!proj::projection_setup(&src_proj) && !src_proj_err)
            { // If the input is a standard projection
                for (int x = 0; x < (int)result_prj.img.width(); x++)
                {
                    for (int y = 0; y < (int)result_prj.img.height(); y++)
                    {
                        double lon, lat, x2, y2; // To allow OMP...
                        if (proj::projection_perform_inv(&trg_proj, x, y, &lon, &lat))
                            continue;
                        if (proj::projection_perform_fwd(&src_proj, lon, lat, &x2, &y2))
                            continue;
                        transposePixel(op.img, result_prj.img, x2, y2, x, y);
                    }
                    if (progress != nullptr)
                        *progress = float(x) / float(result_prj.img.width());
                }

                proj::projection_free(&src_proj);
            }
            else
            { // Otherwise we assume it to be a satellite warp
                logger->info("Warp?");
                if (op.use_old_algorithm)
                {
                    logger->info("Old?");
                }
                else
                {
                    logger->info("New?");

                    warp::WarpOperation operation;
                    operation.ground_control_points = satdump::gcp_compute::compute_gcps(op.source_prj_info, op.img.width(), op.img.height());
                    operation.input_image = op.img;
                    operation.output_rgba = true;
                    // TODO : CHANGE!!!!!!
                    int l_width = std::max<int>(op.img.width(), 512) * 10;
                    operation.output_width = l_width;
                    operation.output_height = l_width / 2;

                    logger->trace("Warping size %dx%d", l_width, l_width / 2);

                    satdump::warp::WarpResult result = satdump::warp::performSmartWarp(operation, progress);

                    //  warped_image = result.output_image;
                    //  tl_lon = result.top_left.lon;
                    //  tl_lat = result.top_left.lat;
                    //  br_lon = result.bottom_right.lon;
                    //  br_lat = result.bottom_right.lat;
                    src_proj = proj::projection_t();
                    src_proj.type = proj::ProjType_Equirectangular;
                    src_proj.proj_offset_x = result.top_left.lon;
                    src_proj.proj_offset_y = result.top_left.lat;
                    src_proj.proj_scalar_x = -(result.top_left.lon - result.bottom_right.lon) / double(result.output_image.width());
                    src_proj.proj_scalar_y = (result.top_left.lat - result.bottom_right.lat) / double(result.output_image.height());

                    result.output_image.save_jpeg("intermediate.jpg");
                    logger->debug("\n%d", nlohmann::json(src_proj).dump(4));

                    if (!proj::projection_setup(&src_proj) && !src_proj_err)
                    { // If the input is a standard projection
                        for (int x = 0; x < (int)result_prj.img.width(); x++)
                        {
                            for (int y = 0; y < (int)result_prj.img.height(); y++)
                            {
                                double lon, lat, x2, y2; // To allow OMP...
                                if (proj::projection_perform_inv(&trg_proj, x, y, &lon, &lat))
                                    continue;
                                if (proj::projection_perform_fwd(&src_proj, lon, lat, &x2, &y2))
                                    continue;
                                transposePixel(result.output_image, result_prj.img, x2, y2, x, y);
                            }
                            if (progress != nullptr)
                                *progress = float(x) / float(result_prj.img.width());
                        }

                        proj::projection_free(&src_proj);
                    }
                }
            }

            proj::projection_free(&trg_proj);
            return result_prj;
        }

        std::function<std::pair<int, int>(double, double, int, int)> setupProjectionFunction(int width, int height,
                                                                                             nlohmann::json params,
                                                                                             bool rotate)
        {
            std::shared_ptr<proj::projection_t> proj(new proj::projection_t(), [](proj::projection_t *obj)
                                                     { proj::projection_free(obj); delete obj; });

            bool proj_err = false;
            try
            {
                *proj = params;
            }
            catch (std::exception &e)
            {
                proj_err = true;
                printf("ERROR PARSING PROJ!!!!!!\n");
            }

            printf("INIT PROJ!!!!!!\n");

            if (!proj::projection_setup(proj.get()) && !proj_err)
            {
                printf("------------------- Using New Proj\n");
                return [proj](double lat, double lon, int, int) mutable -> std::pair<int, int>
                {
                    double x, y;
                    proj::projection_perform_fwd(proj.get(), lon, lat, &x, &y);
                    return {(int)x, (int)y};
                };
            }
            else
            {
                auto gcps = gcp_compute::compute_gcps(params, width, height);
                std::shared_ptr<projection::TPSTransform> transform = std::make_shared<projection::TPSTransform>();
                if (transform->init(gcps, true, false))
                    std::runtime_error("Error generating TPS!");
                return [transform, rotate](double lat, double lon, int map_height, int map_width) mutable -> std::pair<int, int>
                {
                    double x, y;
                    transform->forward(lon, lat, x, y);

                    if (x < 0 || x > map_width)
                        return {-1, -1};
                    if (y < 0 || y > map_height)
                        return {-1, -1};

                    if (rotate)
                    {
                        x = (map_width - 1) - x;
                        y = (map_height - 1) - y;
                    }

                    return {x, y};
                };
            }

            throw std::runtime_error("Invalid projection!!!!");
        }
    }
}