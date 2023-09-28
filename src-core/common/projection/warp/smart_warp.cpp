#include "warp.h"

#include "logger.h"
#include "common/projection/projs/equirectangular.h"
#include "common/geodetic/vincentys_calculations.h"

namespace satdump
{
    namespace warp
    {
        WarpResult performSmartWarp(WarpOperation operation_t)
        {
            image::Image<uint16_t> final_image(operation_t.output_width, operation_t.output_height,
                                               operation_t.output_rgba ? 4 : operation_t.input_image.channels());
            geodetic::projection::EquirectangularProjection projector_final;
            projector_final.init(final_image.width(), final_image.height(), -180, 90, 180, -90);

            double nsegs = 1;

            /// Try to calculate the number of segments to split the data into. All an approximation, but should be ok
            {
                std::vector<satdump::projection::GCP> gcps_curr = operation_t.ground_control_points;
                // Filter GCPs, only keep each first y in x
                std::sort(gcps_curr.begin(), gcps_curr.end(),
                          [&operation_t](auto &el1, auto &el2)
                          {
                              return el1.y * operation_t.input_image.width() + el1.x < el2.y * operation_t.input_image.width() + el2.x;
                          });
                {
                    auto gcps_curr_bkp = gcps_curr;
                    gcps_curr.clear();
                    for (int y = 0; y < gcps_curr_bkp.size() - 1; y++)
                    {
                        auto gcp1 = gcps_curr[y];
                        auto gcp2 = gcps_curr[y + 1];
                        if (gcp1.y != gcp2.y)
                            gcps_curr.push_back(gcp2);
                    }
                }

                std::vector<double> distances;
                for (int y = 0; y < gcps_curr.size() - 1; y++)
                {
                    auto gcp1 = gcps_curr[y];
                    auto gcp2 = gcps_curr[y + 1];

                    auto gcp_dist = geodetic::vincentys_inverse(geodetic::geodetic_coords_t(gcp1.lat, gcp1.lon, 0), geodetic::geodetic_coords_t(gcp2.lat, gcp2.lon, 0));
                    distances.push_back(gcp_dist.distance);
                }

                std::sort(distances.begin(), distances.end());

                double media_dist = distances[distances.size() / 2];

                logger->critical("MEDIAN DISTANCE IS %f", media_dist);
                logger->critical("TOTAL AVG DISTANCE IS %f", media_dist * gcps_curr.size());

                logger->critical("SPLITTING INTO %d SEGMENTS", int((media_dist * gcps_curr.size()) / 3000));

                nsegs = int((media_dist * gcps_curr.size()) / 3000);
                if (nsegs == 0)
                    nsegs = 1;
                // return 0;
            }

            struct SegmentConfig
            {
                int y_start;
                int y_end;
                int shift_lon;
                int shift_lat;
                std::vector<satdump::projection::GCP> gcps;
            };

            std::vector<SegmentConfig> segmentConfigs;

            // GENERATE SEGMENTS
            for (int segment = 0; segment < nsegs; segment++)
            {
                auto generateSeg = [&segmentConfigs, &operation_t](int start, int end, bool start_overlap, bool end_overlap)
                {
                    SegmentConfig scfg;

                    scfg.y_start = start; // (segment / nsegs) * operation_t.input_image.height();     //- 150;
                    scfg.y_end = end;     //((segment + 1) / nsegs) * operation_t.input_image.height(); //+ 150;

                    logger->warn("%d %d", scfg.y_start, scfg.y_end);

                    {
                        int overlap = 0;
                    find_second_gcp:
                        int min_gcp_diff_start = std::numeric_limits<int>::max(); // Find last GCP before start
                        int min_gcp_diff_end = std::numeric_limits<int>::max();   // Find first GCP after end
                        for (auto gcp : operation_t.ground_control_points)
                        {
                            int diffs = scfg.y_start - gcp.y;
                            if (diffs > 0 && diffs < min_gcp_diff_start)
                                min_gcp_diff_start = diffs;
                            int diffe = gcp.y - scfg.y_end;
                            if (diffe > 0 && diffe < min_gcp_diff_end)
                                min_gcp_diff_end = diffe;
                        }

                        if (min_gcp_diff_start != std::numeric_limits<int>::max() && start_overlap)
                            scfg.y_start -= min_gcp_diff_start + 1;
                        if (min_gcp_diff_end != std::numeric_limits<int>::max() && end_overlap)
                            scfg.y_end += min_gcp_diff_end + 1;

                        if (overlap++ < 1)
                            goto find_second_gcp;
                    }

                    logger->warn("%d %d", scfg.y_start, scfg.y_end);

                    if (scfg.y_start < 0)
                        scfg.y_start = 0;
                    if (scfg.y_end > operation_t.input_image.height())
                        scfg.y_end = operation_t.input_image.height();

                    auto gcps_old = operation_t.ground_control_points;
                    std::vector<satdump::projection::GCP> gcps_final;
                    for (auto gcp : gcps_old)
                    {
                        if (gcp.y >= scfg.y_start && gcp.y < scfg.y_end)
                        {
                            gcp.y -= scfg.y_start;
                            gcps_final.push_back(gcp);
                        }
                    }

                    {
                        double x_total = 0;
                        double y_total = 0;
                        double z_total = 0;

                        for (auto &pt : gcps_final)
                        {
                            x_total += cos(pt.lat * DEG_TO_RAD) * cos(pt.lon * DEG_TO_RAD);
                            y_total += cos(pt.lat * DEG_TO_RAD) * sin(pt.lon * DEG_TO_RAD);
                            z_total += sin(pt.lat * DEG_TO_RAD);
                        }

                        x_total /= gcps_final.size();
                        y_total /= gcps_final.size();
                        z_total /= gcps_final.size();

                        double lon = atan2(y_total, x_total) * RAD_TO_DEG;
                        double hyp = sqrt(x_total * x_total + y_total * y_total);
                        double lat = atan2(z_total, hyp) * RAD_TO_DEG;

                        logger->trace("CENTER LATLON %f %f", lat, lon);

                        scfg.shift_lon = -lon;
                        scfg.shift_lat = 0;
                    }

                    // Check for GCPs near the poles. If any is close, we wanna discard it.
                    for (auto gcp : gcps_final)
                    {
                        auto south_dis = geodetic::vincentys_inverse(geodetic::geodetic_coords_t(gcp.lat, gcp.lon, 0), geodetic::geodetic_coords_t(-90, 0, 0));
                        auto north_dis = geodetic::vincentys_inverse(geodetic::geodetic_coords_t(gcp.lat, gcp.lon, 0), geodetic::geodetic_coords_t(90, 0, 0));

                        if (south_dis.distance < 1000)
                        {
                            // logger->critical("TOO CLOSE TO SOUTH POLE. SHIFT!");
                            scfg.shift_lon = 0;
                            scfg.shift_lat = -90;
                            // continue;
                        }
                        if (north_dis.distance < 1000)
                        {
                            // logger->critical("TOO CLOSE TO NORTH POLE. SHIFT!");
                            scfg.shift_lon = 0;
                            scfg.shift_lat = 90;
                            // continue;
                        }
                    }

                    scfg.gcps = gcps_final;

                    segmentConfigs.push_back(scfg);
                };

                int y_start = (segment / nsegs) * operation_t.input_image.height();
                int y_end = ((segment + 1) / nsegs) * operation_t.input_image.height();

                std::vector<satdump::projection::GCP> gcps_curr;
                for (auto gcp : operation_t.ground_control_points)
                {
                    if (gcp.y >= y_start && gcp.y < y_end)
                    {
                        // gcp.y -= y_start;
                        gcps_curr.push_back(gcp);
                    }
                }

                // Filter GCPs, only keep each first y in x
                std::sort(gcps_curr.begin(), gcps_curr.end(),
                          [&operation_t](auto &el1, auto &el2)
                          {
                              return el1.y * operation_t.input_image.width() + el1.x < el2.y * operation_t.input_image.width() + el2.x;
                          });
                {
                    auto gcps_curr_bkp = gcps_curr;
                    gcps_curr.clear();
                    for (int y = 0; y < gcps_curr_bkp.size() - 1; y++)
                    {
                        auto gcp1 = gcps_curr[y];
                        auto gcp2 = gcps_curr[y + 1];
                        if (gcp1.y != gcp2.y)
                            gcps_curr.push_back(gcp2);
                    }
                }

                int cutPosition = -1;
                for (int y = 0; y < gcps_curr.size() - 1; y++)
                {
                    auto gcp1 = gcps_curr[y];
                    auto gcp2 = gcps_curr[y + 1];

                    auto gcp_dist = geodetic::vincentys_inverse(geodetic::geodetic_coords_t(gcp1.lat, gcp1.lon, 0), geodetic::geodetic_coords_t(gcp2.lat, gcp2.lon, 0));
                    logger->trace("---------------------------------- %f %f DISTANCE IS %f", gcp1.y, gcp1.x, gcp_dist.distance);
                    if (gcp_dist.distance > 2000)
                        cutPosition = gcp2.y;
                }

                if (cutPosition != -1)
                {
                    generateSeg(y_start, cutPosition, true, false);
                    generateSeg(cutPosition, y_end, false, true);
                }
                else
                {
                    generateSeg(y_start, y_end, true, true);
                }
            }

            int scnt = 0;
            // PROCESS SEGMENTS
            for (auto &segmentCfg : segmentConfigs)
            {
                auto operation = operation_t;

                operation.input_image.crop(0, segmentCfg.y_start, operation.input_image.width(), segmentCfg.y_end);
                operation.shift_lon = segmentCfg.shift_lon;
                operation.shift_lat = segmentCfg.shift_lat;
                operation.ground_control_points = segmentCfg.gcps;

                satdump::warp::ImageWarper warper;
                warper.op = operation;
                warper.update();

                satdump::warp::WarpResult result = warper.warp();

                geodetic::projection::EquirectangularProjection projector;
                projector.init(result.output_image.width(), result.output_image.height(), result.top_left.lon, result.top_left.lat, result.bottom_right.lon, result.bottom_right.lat);

#if 0 // DRAW GCPs
                {
                    unsigned short color[4] = {0, 65535, 0, 65535};
                    for (auto gcp : operation.ground_control_points)
                    {
                        auto projfunc = [&projector](float lat, float lon, int, int) -> std::pair<int, int>
                        {
                            int x, y;
                            projector.forward(lon, lat, x, y);
                            return {x, y};
                        };

                        std::pair<int, int> pos = projfunc(gcp.lat, gcp.lon, 0, 0);

                        if (pos.first == -1 || pos.second == -1)
                            continue;

                        result.output_image.draw_circle(pos.first, pos.second, 5, color, true);
                    }
                }
#endif

                float lon = result.top_left.lon, lat = result.top_left.lat;
                int x2, y2;

                projector_final.forward(lon, lat, x2, y2);
                if (x2 == -1 || y2 == -1)
                    ;
                else
                {
                    //  final_image.draw_image(0, result.output_image, x2, y2);
                    logger->critical("POS %d %d %f %f", x2, y2, result.top_left.lon, result.top_left.lat);

                    // int c = 0;
                    int x0 = x2;
                    int y0 = y2;

                    // Get min height and width, mostly for safety
                    int width = std::min<int>(final_image.width(), x0 + result.output_image.width()) - x0;
                    int height = std::min<int>(final_image.height(), y0 + result.output_image.height()) - y0;

                    if (/*c == 0 &&*/ result.output_image.channels() == final_image.channels()) // Special case for non-grayscale images
                    {
                        for (int x = 0; x < width; x++)
                        {
                            for (int y = 0; y < height; y++)
                            {
                                if (y + y0 >= 0 && x + x0 >= 0)
                                {
                                    if (result.output_image.channel(3)[y * result.output_image.width() + x] > 0
                                        /*&& final_image.channel(3)[(y + y0) * final_image.width() + x + x0] == 0*/
                                    )
                                    {
                                        // for (int c = 0; c < 3; c++)
                                        //     final_image.channel(c)[(y + y0) * final_image.width() + x + x0] = ;
                                        for (int ch = 0; ch < 3; ch++)
                                            final_image.channel(ch)[(y + y0) * final_image.width() + x + x0] = result.output_image.channel(ch)[y * result.output_image.width() + x];
                                        final_image.channel(3)[(y + y0) * final_image.width() + x + x0] = 65535;
                                    }
                                }
                            }
                        }
                    }
                }

                // result.output_image.save_img("projtest/test" + std::to_string((scnt++) + 1));
            }

            return {final_image,
                    {0, 0, -180, 90},
                    {final_image.width() - 1, 0, 180, 90},
                    {final_image.width() - 1, final_image.height() - 1, 180, -90},
                    {0, final_image.height() - 1, -180, -90}};
        }
    }
}