#include "reprojector.h"

#include "logger.h"
#include "core/exception.h"
#include "common/image/meta.h"

#include "common/projection/projs2/proj_json.h"

#include "common/projection/warp/warp.h"

#include "projection/projection.h"

#include "raytrace/gcp_compute.h"

namespace satdump
{
    namespace proj
    {
        inline void transposePixel(image::Image &in, image::Image &out, double ix, double iy, int ox, int oy)
        {
            if (ix >= (int)in.width() || iy >= (int)in.height() || ix < 0 || iy < 0)
                return;
            if (ox >= (int)out.width() || oy >= (int)out.height() || ox < 0 || oy < 0)
                return;

            if (in.channels() == 4)
            {
                for (int c = 0; c < in.channels(); c++)
                    out.set(c, oy * out.width() + ox, in.get_pixel_bilinear(c, ix, iy)); // in.channel(c)[iy * in.width() + ix];
            }
            else if (in.channels() == 3)
            {
                for (int c = 0; c < in.channels(); c++)
                    out.set(c, oy * out.width() + ox, c == 3 ? 65535 : in.get_pixel_bilinear(c, ix, iy)); // in.channel(c)[iy * in.width() + ix];
                if (out.channels() == 4)
                    out.set(3, oy * out.width() + ox, 65535);
            }
            else if (in.channels() == 1) //|| in.channels() == 2)
            {
                for (int c = 0; c < out.channels(); c++)
                    out.set(c, oy * out.width() + ox, in.get_pixel_bilinear(0, ix, iy)); // in.channel(0)[iy * in.width() + ix];
                if (out.channels() == 4)
                    out.set(3, oy * out.width() + ox, 65535);
            }
            else
            {
                for (int c = 0; c < in.channels(); c++)
                    out.set(c, oy * out.width() + ox, c == 3 ? 65535 : in.get_pixel_bilinear(0, ix, iy)); // in.channel(0)[iy * in.width() + ix];
                if (out.channels() == 4)
                    out.set(3, oy * out.width() + ox, 65535);
            }
        }

        image::Image reproject(ReprojectionOperation &op, float *progress)
        {
            if (op.img->size() == 0)
                throw satdump_exception("Can't reproject an empty image!");
            if (!image::has_metadata_proj_cfg(*op.img))
                throw satdump_exception("Can't reproject an image with no proj config!");

            // TODOIMG for now can only project 16-bits
            image::Image op_img16;
            if (op.img->depth() != 16)
            {
                op_img16 = op.img->to16bits();
                image::set_metadata(op_img16, image::get_metadata(*op.img));
                op.img = &op_img16;
            }

            image::Image result_img(16, op.output_width, op.output_height, 4);

            // Attempt to init target proj
            Projection trg_proj;
            //            proj::projection_t trg_proj;
            bool trg_proj_err = false;
            try
            {
                trg_proj = op.target_prj_info;
            }
            catch (std::exception &)
            {
                trg_proj_err = true;
            }

            if (!trg_proj.init(false, true) || trg_proj_err)
            {
                logger->error("Invalid target projection? Could not init! %d : \n%s", (int)trg_proj_err, op.target_prj_info.dump(4).c_str());
                return result_img;
            }

            // Attempt to init source proj
            Projection src_proj;
            //            proj::projection_t src_proj;
            bool src_proj_err = false;
            try
            {
                src_proj = image::get_metadata_proj_cfg(*op.img);
            }
            catch (std::exception &)
            {
                src_proj_err = true;
            }

            if (src_proj.init(true, false) && !src_proj_err &&
                src_proj.getInvType() != Projection::PROJ_RAYTRACER /*TODOREWORK*/)
            { // If the input is a standard projection
                geodetic::geodetic_coords_t pos;
                for (int x = 0; x < (int)result_img.width(); x++)
                {
                    for (int y = 0; y < (int)result_img.height(); y++)
                    {
                        double lon, lat, x2, y2;         // To allow OMP...
                        if (trg_proj.inverse(x, y, pos)) //  proj::projection_perform_inv(&trg_proj, x, y, &lon, &lat))
                            continue;
                        if (src_proj.forward(pos, x2, y2)) // proj::projection_perform_fwd(&src_proj, lon, lat, &x2, &y2))
                            continue;
                        transposePixel(*op.img, result_img, x2, y2, x, y);
                    }
                    if (progress != nullptr)
                        *progress = float(x) / float(result_img.width());
                }

                //                proj::projection_free(&src_proj);
            }
            else
            { // Otherwise we assume it to be a satellite warp
                if (op.use_old_algorithm)
                {
#if 0
                     // This is garbage, but robust to noise garbage!
                    logger->info("Using old algorithm...!");

                    auto src_proj_cfg = image::get_metadata_proj_cfg(*op.img);

                    // Init
                    std::function<std::pair<int, int>(float, float, int, int)> projectionFunction = setupProjectionFunction(result_img.width(),
                                                                                                                            result_img.height(),
                                                                                                                            op.target_prj_info,
                                                                                                                            {});

                    TLE tle;
                    if (src_proj_cfg.contains("tle"))
                        tle = src_proj_cfg["tle"];
                    else
                        throw satdump_exception("Reproj : Could not get TLE!");

                    nlohmann::ordered_json timestamps;
                    if (src_proj_cfg.contains("timestamps"))
                        timestamps = src_proj_cfg["timestamps"];
                    else
                        throw satdump_exception("Reproj : Could not get timestamps!");

                    // Reproj
                    // ""TPS""" (Mostly LEO)
                    {
                        std::shared_ptr<SatelliteProjection> sat_proj_src = get_sat_proj(src_proj_cfg, tle, timestamps);
                        bool direction = false;

                        double ratio_x = round((double)sat_proj_src->img_size_x / (double)op.img->width());
                        double ratio_y = round((double)sat_proj_src->img_size_y / (double)op.img->height());

                        for (int currentScan = 0; currentScan < (int)op.img->height(); currentScan++)
                        {
                            // Now compute each pixel's lat / lon and plot it
                            for (double px = 0; px < op.img->width() - 1; px += 1)
                            {
                                geodetic::geodetic_coords_t coords1, coords2, coords3;
                                bool ret1 = sat_proj_src->get_position(px * ratio_x, currentScan * ratio_y, coords1);
                                bool ret2 = sat_proj_src->get_position((px + 1) * ratio_x, currentScan * ratio_y, coords2);
                                sat_proj_src->get_position(px * ratio_x, (currentScan + 1) * ratio_y, coords3);

                                if (ret1 || ret2)
                                    continue;

                                direction = coords1.lat > coords3.lat;

                                std::pair<float, float> map_cc1 = projectionFunction(coords1.lat, coords1.lon, result_img.height(), result_img.width());
                                std::pair<float, float> map_cc2 = projectionFunction(coords2.lat, coords2.lon, result_img.height(), result_img.width());

                                std::vector<double> color = {0, 0, 0, 0};
                                if (op.img->channels() >= 3)
                                {
                                    color[0] = op.img->getf(0, currentScan * op.img->width() + int(px));
                                    color[1] = op.img->getf(1, currentScan * op.img->width() + int(px));
                                    color[2] = op.img->getf(2, currentScan * op.img->width() + int(px));
                                    color[3] = 1;
                                }
                                else
                                {
                                    color[0] = op.img->getf(currentScan * op.img->width() + int(px));
                                    color[1] = op.img->getf(currentScan * op.img->width() + int(px));
                                    color[2] = op.img->getf(currentScan * op.img->width() + int(px));
                                    color[3] = 1;
                                }

                                // if (color[0] == 0 && color[1] == 0 && color[2] == 0) // Skip Black
                                //     continue;

                                // This seems to glitch out sometimes... Need to check
                                double maxSize = result_img.width() / 100.0;
                                if (abs(map_cc1.first - map_cc2.first) < maxSize && abs(map_cc1.second - map_cc2.second) < maxSize)
                                {
                                    /*
                                    Using a circle and radius from the next sample is most likely not the best way, computing the instrument's
                                    actual FOV at each pixel and so would be a better approach... But well, this one has the benefit of being
                                    fast.
                                    I guess time will tell how reliable that approximation is.
                                    */
                                    double circle_radius = sqrt(pow(int(map_cc1.first - map_cc2.first), 2) + pow(int(map_cc1.second - map_cc2.second), 2));
                                    result_img.draw_circle(map_cc1.first, map_cc1.second + (direction ? (circle_radius * 2.0) : (circle_radius * -2.0)), ceil(circle_radius), color, true); //, 0.4 * opacity);
                                }

                                // projected_image.draw_point(map_cc1.first, map_cc1.second, color, opacity);
                            }

                            if (progress != nullptr)
                                *progress = float(currentScan) / float(op.img->height());
                            // logger->critical("%d/%d", currentScan, image.height());
                        }
                    }
#endif
                }
                else
                {
                    auto prj_cfg = image::get_metadata_proj_cfg(*op.img);
                    prj_cfg["width"] = op.img->width();
                    prj_cfg["height"] = op.img->height();
                    warp::WarpOperation operation;
                    operation.ground_control_points = compute_gcps(prj_cfg);
                    operation.input_image = op.img;
                    operation.output_rgba = true;
                    // TODO : CHANGE!!!!!!
                    int l_width = prj_cfg.contains("f_width") ? prj_cfg["f_width"].get<int>() : std::max<int>(op.img->width(), 512) * 10;
                    operation.output_width = l_width;
                    operation.output_height = l_width / 2;

                    logger->trace("Warping size %dx%d", l_width, l_width / 2);

                    satdump::warp::WarpResult result = satdump::warp::performSmartWarp(operation, progress);

                    ::proj::projection_t src_proj = ::proj::projection_t();
                    src_proj.type = ::proj::ProjType_Equirectangular;
                    src_proj.proj_offset_x = result.top_left.lon;
                    src_proj.proj_offset_y = result.top_left.lat;
                    src_proj.proj_scalar_x = (result.bottom_right.lon - result.top_left.lon) / double(result.output_image.width());
                    src_proj.proj_scalar_y = (result.bottom_right.lat - result.top_left.lat) / double(result.output_image.height());

                    if (!::proj::projection_setup(&src_proj) && !src_proj_err)
                    { // If the input is a standard projection
                        geodetic::geodetic_coords_t pos;
                        for (int x = 0; x < (int)result_img.width(); x++)
                        {
                            for (int y = 0; y < (int)result_img.height(); y++)
                            {
                                double lon, lat, x2, y2;         // To allow OMP...
                                if (trg_proj.inverse(x, y, pos)) // proj::projection_perform_inv(&trg_proj, x, y, &lon, &lat))
                                    continue;
                                if (::proj::projection_perform_fwd(&src_proj, pos.lon, pos.lat, &x2, &y2))
                                    continue;
                                transposePixel(result.output_image, result_img, x2, y2, x, y);
                            }
                            if (progress != nullptr)
                                *progress = float(x) / float(result_img.width());
                        }

                        ::proj::projection_free(&src_proj);
                    }
                }
            }

            image::set_metadata_proj_cfg(result_img, trg_proj);

            //            proj::projection_free(&trg_proj);
            return result_img;
        }
    }
}