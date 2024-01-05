#include "reprojector.h"
#include "logger.h"
#include "warp/warp.h"
#include "gcp_compute/gcp_compute.h"

#include "sat_proj/sat_proj.h"

#include "projs/equirectangular.h"
#include "projs/mercator.h"
#include "projs/stereo.h"
#include "projs/tpers.h"
#include "projs/azimuthal_equidistant.h"

#include "sat_proj/geo_projection.h"
#include "projs/tps_transform.h"
#include "reproj/reproj.h"

namespace satdump
{
    namespace reprojection
    {
        ProjectionResult reproject(ReprojectionOperation &op, float *progress)
        {
            ProjectionResult result_prj;

            if (op.img.size() == 0)
                throw std::runtime_error("Can't reproject an empty image!");

            result_prj.img.init(op.output_width, op.output_height, 4);
            result_prj.settings = op.target_prj_info;

            auto &image = op.img;
            auto &projected_image = result_prj.img;

            logger->info("Using new algorithm...");

            // Here, we first project to an equirectangular target
            image::Image<uint16_t> warped_image;
            float tl_lon = 0, tl_lat = 0;
            float br_lon = 0, br_lat = 0;

            // Reproject to equirect
            if (op.source_prj_info["type"] == "equirectangular")
            {
                warped_image = op.img;
                tl_lon = op.source_prj_info["tl_lon"].get<float>();
                tl_lat = op.source_prj_info["tl_lat"].get<float>();
                br_lon = op.source_prj_info["br_lon"].get<float>();
                br_lat = op.source_prj_info["br_lat"].get<float>();
            }
            else if (op.source_prj_info["type"] == "mercator")
            {
                int g_width = op.img.width();               // Should be reasonnable for now!
                warped_image.init(g_width, g_width / 2, 4); // TODO : CHANGE!!!!!

                tl_lon = -180;
                tl_lat = 85.06;
                br_lon = 180;
                br_lat = -85.06;

                reproj::reproject_merc_to_equ(op.img,
                                              // op.source_prj_info["lon"].get<float>(),
                                              // op.source_prj_info["alt"].get<double>(),
                                              // op.source_prj_info["scale_x"].get<float>(), op.source_prj_info["scale_y"].get<float>(),
                                              // op.source_prj_info["offset_x"].get<float>(), op.source_prj_info["offset_y"].get<float>(),
                                              // op.source_prj_info["sweep_x"].get<bool>(),
                                              warped_image,
                                              tl_lon, tl_lat,
                                              br_lon, br_lat,
                                              progress);
            }
            else if (op.source_prj_info["type"] == "geos")
            {
                int g_width = op.img.width() * 2;           // Should be reasonnable for now!
                warped_image.init(g_width, g_width / 2, 4); // TODO : CHANGE!!!!!

                tl_lon = -180;
                tl_lat = 90;
                br_lon = 180;
                br_lat = -90;

                reproj::reproject_geos_to_equ(op.img,
                                              op.source_prj_info["lon"].get<float>(),
                                              op.source_prj_info["alt"].get<double>(),
                                              op.source_prj_info["scale_x"].get<float>(), op.source_prj_info["scale_y"].get<float>(),
                                              op.source_prj_info["offset_x"].get<float>(), op.source_prj_info["offset_y"].get<float>(),
                                              op.source_prj_info["sweep_x"].get<bool>(),
                                              warped_image,
                                              tl_lon, tl_lat,
                                              br_lon, br_lat,
                                              progress);
            }
            else // Means it's a TPS-handled warp.
            {
                if (op.use_old_algorithm)
                {
                    // Source projs
                    std::shared_ptr<SatelliteProjection> sat_proj_src;

                    // Init
                    std::function<std::pair<int, int>(float, float, int, int)> projectionFunction = setupProjectionFunction(projected_image.width(),
                                                                                                                            projected_image.height(),
                                                                                                                            op.target_prj_info,
                                                                                                                            {});

                    nlohmann::json mtd = op.source_prj_info.contains("metadata") ? op.source_prj_info["metadata"] : nlohmann::json();
                    int img_x_offset = 0;
                    if (mtd.contains("img_offset_x"))
                        img_x_offset = mtd["img_offset_x"];

                    TLE tle;
                    if (mtd.contains("tle"))
                        tle = mtd["tle"];
                    else
                        throw std::runtime_error("Could not get TLE!");

                    nlohmann::ordered_json timestamps;
                    if (mtd.contains("timestamps"))
                        timestamps = mtd["timestamps"];
                    else
                        throw std::runtime_error("Could not get timestamps!");

                    // Reproj
                    // ""TPS""" (Mostly LEO)
                    {
                        sat_proj_src = get_sat_proj(op.source_prj_info, tle, timestamps);
                        bool direction = false;

                        double ratio_x = round((double)sat_proj_src->img_size_x / (double)op.img.width());
                        double ratio_y = round((double)sat_proj_src->img_size_y / (double)op.img.height());

                        for (int currentScan = 0; currentScan < (int)image.height(); currentScan++)
                        {
                            // Now compute each pixel's lat / lon and plot it
                            for (double px = 0; px < image.width() - 1; px += 1)
                            {
                                geodetic::geodetic_coords_t coords1, coords2, coords3;
                                bool ret1 = sat_proj_src->get_position(px * ratio_x, currentScan * ratio_y, coords1);
                                bool ret2 = sat_proj_src->get_position((px + 1) * ratio_x, currentScan * ratio_y, coords2);
                                sat_proj_src->get_position((px - img_x_offset) * ratio_x, (currentScan + 1) * ratio_y, coords3);

                                if (ret1 || ret2)
                                    continue;

                                direction = coords1.lat > coords3.lat;

                                std::pair<float, float> map_cc1 = projectionFunction(coords1.lat, coords1.lon, projected_image.height(), projected_image.width());
                                std::pair<float, float> map_cc2 = projectionFunction(coords2.lat, coords2.lon, projected_image.height(), projected_image.width());

                                uint16_t color[4] = {0, 0, 0, 0};
                                if (image.channels() >= 3)
                                {
                                    color[0] = image.channel(0)[currentScan * image.width() + int(px)];
                                    color[1] = image.channel(1)[currentScan * image.width() + int(px)];
                                    color[2] = image.channel(2)[currentScan * image.width() + int(px)];
                                    color[3] = 65535;
                                }
                                else
                                {
                                    color[0] = image[currentScan * image.width() + int(px)];
                                    color[1] = image[currentScan * image.width() + int(px)];
                                    color[2] = image[currentScan * image.width() + int(px)];
                                    color[3] = 65535;
                                }

                                // if (color[0] == 0 && color[1] == 0 && color[2] == 0) // Skip Black
                                //     continue;

                                // This seems to glitch out sometimes... Need to check
                                double maxSize = projected_image.width() / 100.0;
                                if (abs(map_cc1.first - map_cc2.first) < maxSize && abs(map_cc1.second - map_cc2.second) < maxSize)
                                {
                                    /*
                                    Using a circle and radius from the next sample is most likely not the best way, computing the instrument's
                                    actual FOV at each pixel and so would be a better approach... But well, this one has the benefit of being
                                    fast.
                                    I guess time will tell how reliable that approximation is.
                                    */
                                    double circle_radius = sqrt(pow(int(map_cc1.first - map_cc2.first), 2) + pow(int(map_cc1.second - map_cc2.second), 2));
                                    projected_image.draw_circle(map_cc1.first, map_cc1.second + (direction ? (circle_radius * 2.0) : (circle_radius * -2.0)), ceil(circle_radius), color, true); //, 0.4 * opacity);
                                }

                                // projected_image.draw_point(map_cc1.first, map_cc1.second, color, opacity);
                            }

                            if (progress != nullptr)
                                *progress = float(currentScan) / float(image.height());
                            // logger->critical("%d/%d", currentScan, image.height());
                        }
                    }
                }
                else
                {
                    warp::WarpOperation operation;
                    operation.ground_control_points = satdump::gcp_compute::compute_gcps(op.source_prj_info, op.img.width(), op.img.height());
                    operation.input_image = op.img;
                    operation.output_rgba = true;
                    // TODO : CHANGE!!!!!!
                    int l_width = std::max<int>(op.img.width(), 512) * 10;
                    operation.output_width = l_width;
                    operation.output_height = l_width / 2;

                    logger->trace("Warping size %dx%d", l_width, l_width / 2);

#if 0
                    satdump::warp::ImageWarper warper;
                    warper.op = operation;
                    warper.update();

                    satdump::warp::WarpResult result = warper.warp();
#else
                    satdump::warp::WarpResult result = satdump::warp::performSmartWarp(operation, progress);
#endif

                    warped_image = result.output_image;
                    tl_lon = result.top_left.lon;
                    tl_lat = result.top_left.lat;
                    br_lon = result.bottom_right.lon;
                    br_lat = result.bottom_right.lat;
                }
            }

            logger->info("Reprojecting to target...");

            // Reproject to target
            if (op.target_prj_info["type"] == "equirectangular")
            {
                geodetic::projection::EquirectangularProjection equi_proj;
                equi_proj.init(projected_image.width(), projected_image.height(),
                               op.target_prj_info["tl_lon"].get<float>(), op.target_prj_info["tl_lat"].get<float>(),
                               op.target_prj_info["br_lon"].get<float>(), op.target_prj_info["br_lat"].get<float>());
                geodetic::projection::EquirectangularProjection equi_proj_src;
                equi_proj_src.init(warped_image.width(), warped_image.height(), tl_lon, tl_lat, br_lon, br_lat);

                float lon, lat;
                int x2, y2;
                for (int x = 0; x < (int)projected_image.width(); x++)
                {
                    for (int y = 0; y < (int)projected_image.height(); y++)
                    {
                        equi_proj.reverse(x, y, lon, lat);
                        if (lon == -1 || lat == -1)
                            continue;
                        equi_proj_src.forward(lon, lat, x2, y2);
                        if (x2 == -1 || y2 == -1)
                            continue;

                        if (warped_image.channels() == 4)
                        {
                            for (int c = 0; c < warped_image.channels(); c++)
                                projected_image.channel(c)[y * projected_image.width() + x] = warped_image.channel(c)[y2 * warped_image.width() + x2];
                        }
                        else if (warped_image.channels() == 3)
                        {
                            for (int c = 0; c < warped_image.channels(); c++)
                                projected_image.channel(c)[y * projected_image.width() + x] = c == 3 ? 65535 : warped_image.channel(c)[y2 * warped_image.width() + x2];
                            if (projected_image.channels() == 4)
                                projected_image.channel(3)[y * projected_image.width() + x] = 65535;
                        }
                        else
                        {
                            for (int c = 0; c < warped_image.channels(); c++)
                                projected_image.channel(c)[y * projected_image.width() + x] = c == 3 ? 65535 : warped_image.channel(0)[y2 * warped_image.width() + x2];
                            if (projected_image.channels() == 4)
                                projected_image.channel(3)[y * projected_image.width() + x] = 65535;
                        }
                    }

                    if (progress != nullptr)
                        *progress = float(x) / float(projected_image.width());
                }
            }
            else if (op.target_prj_info["type"] == "mercator")
            {
                geodetic::projection::MercatorProjection merc_proj;
                merc_proj.init(projected_image.width(), projected_image.height() /*,
                                op.target_prj_info["tl_lon"].get<float>(), op.target_prj_info["tl_lat"].get<float>(),
                                op.target_prj_info["br_lon"].get<float>(), op.target_prj_info["br_lat"].get<float>()*/
                );
                geodetic::projection::EquirectangularProjection equi_proj_src;
                equi_proj_src.init(warped_image.width(), warped_image.height(), tl_lon, tl_lat, br_lon, br_lat);

                float lon, lat;
                int x2, y2;
                for (int x = 0; x < (int)projected_image.width(); x++)
                {
                    for (int y = 0; y < (int)projected_image.height(); y++)
                    {
                        merc_proj.reverse(x, y, lon, lat);
                        if (lon == -1 || lat == -1)
                            continue;
                        equi_proj_src.forward(lon, lat, x2, y2);
                        if (x2 == -1 || y2 == -1)
                            continue;

                        if (warped_image.channels() == 4)
                            for (int c = 0; c < warped_image.channels(); c++)
                                projected_image.channel(c)[y * projected_image.width() + x] = warped_image.channel(c)[y2 * warped_image.width() + x2];
                        else if (warped_image.channels() == 3)
                            for (int c = 0; c < warped_image.channels(); c++)
                                projected_image.channel(c)[y * projected_image.width() + x] = c == 3 ? 65535 : warped_image.channel(c)[y2 * warped_image.width() + x2];
                        else
                            for (int c = 0; c < warped_image.channels(); c++)
                                projected_image.channel(c)[y * projected_image.width() + x] = c == 3 ? 65535 : warped_image.channel(0)[y2 * warped_image.width() + x2];
                    }

                    if (progress != nullptr)
                        *progress = float(x) / float(projected_image.width());
                }
            }
            else if (op.target_prj_info["type"] == "stereo")
            {
                reproj::reproject_equ_to_stereo(warped_image,
                                                tl_lon, tl_lat, br_lon, br_lat, projected_image,
                                                op.target_prj_info["center_lat"].get<float>(),
                                                op.target_prj_info["center_lon"].get<float>(),
                                                op.target_prj_info["scale"].get<float>(),
                                                progress);
            }
            else if (op.target_prj_info["type"] == "tpers")
            {
                reproj::reproject_equ_to_tpers(warped_image,
                                               tl_lon, tl_lat, br_lon, br_lat, projected_image,
                                               op.target_prj_info["alt"].get<float>() * 1000,
                                               op.target_prj_info["lon"].get<float>(),
                                               op.target_prj_info["lat"].get<float>(),
                                               op.target_prj_info["ang"].get<float>(),
                                               op.target_prj_info["azi"].get<float>(),
                                               progress);
            }
            else if (op.target_prj_info["type"] == "azeq")
            {
                reproj::reproject_equ_to_azeq(warped_image,
                                              tl_lon, tl_lat, br_lon, br_lat, projected_image,
                                              op.target_prj_info["lon"].get<float>(),
                                              op.target_prj_info["lat"].get<float>(),
                                              progress);
            }

            return result_prj;
        }

        std::function<std::pair<int, int>(float, float, int, int)> setupProjectionFunction(int width, int height,
                                                                                           nlohmann::json params,
                                                                                           bool rotate)
        {
            if (params["type"] == "equirectangular")
            {
                geodetic::projection::EquirectangularProjection projector;
                projector.init(width, height,
                               params["tl_lon"].get<float>(), params["tl_lat"].get<float>(),
                               params["br_lon"].get<float>(), params["br_lat"].get<float>());

                return [projector, rotate](float lat, float lon, int, int) mutable -> std::pair<int, int>
                {
                    int x, y;
                    projector.forward(lon, lat, x, y);
                    return {x, y};
                };
            }
            else if (params["type"] == "mercator")
            {
                geodetic::projection::MercatorProjection projector;
                projector.init(width, height /*,
                                params["tl_lon"].get<float>(), params["tl_lat"].get<float>(),
                                params["br_lon"].get<float>(), params["br_lat"].get<float>()*/
                );

                return [projector, rotate](float lat, float lon, int, int) mutable -> std::pair<int, int>
                {
                    int x, y;
                    projector.forward(lon, lat, x, y);
                    return {x, y};
                };
            }
            else if (params["type"] == "stereo")
            {
                geodetic::projection::StereoProjection stereo_proj;
                stereo_proj.init(params["center_lat"].get<float>(), params["center_lon"].get<float>());
                float stereo_scale = params["scale"].get<float>();

                return [stereo_proj, stereo_scale, rotate](float lat, float lon, int map_height, int map_width) mutable -> std::pair<int, int>
                {
                    double x = 0;
                    double y = 0;
                    stereo_proj.forward(lon, lat, x, y);
                    x *= map_width / stereo_scale;
                    y *= map_height / stereo_scale;
                    return {x + (map_width / 2), map_height - (y + (map_height / 2))};
                };
            }
            else if (params["type"] == "tpers")
            {
                geodetic::projection::TPERSProjection tpers_proj;
                tpers_proj.init(params["alt"].get<float>() * 1000,
                                params["lon"].get<float>(),
                                params["lat"].get<float>(),
                                params["ang"].get<float>(),
                                params["azi"].get<float>());

                return [tpers_proj, rotate](float lat, float lon, int map_height, int map_width) mutable -> std::pair<int, int>
                {
                    double x, y;
                    tpers_proj.forward(lon, lat, x, y);
                    x *= map_width / 2;
                    y *= map_height / 2;
                    int finalx = x + (map_width / 2);
                    int finaly = map_height - (y + (map_height / 2));
                    if (finalx < 0 || finaly < 0)
                        return {-1, -1};
                    if (finalx >= map_width || finaly >= map_height)
                        return {-1, -1};
                    return {finalx, finaly};
                };
            }
            else if (params["type"] == "geos")
            {
                geodetic::projection::GEOProjector geo_proj(params["lon"].get<float>(),
                                                            params["alt"].get<double>(),
                                                            width, height,
                                                            params["scale_x"].get<float>(), params["scale_y"].get<float>(),
                                                            params["offset_x"].get<float>(), params["offset_y"].get<float>(),
                                                            params["sweep_x"].get<bool>());
                return [geo_proj, rotate](float lat, float lon, int map_height, int map_width) mutable -> std::pair<int, int>
                {
                    int x;
                    int y;
                    geo_proj.forward(lon, lat, x, y);

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
            else if (params["type"] == "azeq")
            {
                geodetic::projection::AzimuthalEquidistantProjection eqaz_proj;
                eqaz_proj.init(width, height, params["lon"].get<float>(), params["lat"].get<float>());
                return [eqaz_proj, rotate](float lat, float lon, int /*map_height*/, int /*map_width*/) mutable -> std::pair<int, int>
                {
                    int x, y;
                    eqaz_proj.forward(lon, lat, x, y);
                    return {x, y};
                };
            }
            else
            {
                auto gcps = gcp_compute::compute_gcps(params, width, height);
                std::shared_ptr<projection::TPSTransform> transform = std::make_shared<projection::TPSTransform>();
                if (transform->init(gcps, true, false))
                    std::runtime_error("Error generating TPS!");
                return [transform, rotate](float lat, float lon, int map_height, int map_width) mutable -> std::pair<int, int>
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