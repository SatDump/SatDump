#include "warp_bkd.h"

#include "logger.h"
#include "core/exception.h"
#include "common/projection/projs/equirectangular.h"
#include "common/geodetic/vincentys_calculations.h"

#define MAX_IMAGE_RAM_USAGE 4e9 // 4GB of RAM max
#define SEGMENT_SIZE_KM 3000    // Average segment size to try and keep as max
#define MIN_POLE_DISTANCE 1000  // Maximum distance at which to consider we're working over a pole
// #define SEGMENT_CUT_DISTANCE_KM 1000 // Distance at which we consider a segment needs to be cut. TODO : tune!

namespace satdump
{
    namespace warp
    {
        void ensureMemoryLimit(satdump::warp::WarpCropSettings &crop_set, WarpOperation &operation_t, int nchannels, size_t mem_limit)
        {
        recheck_memory:
            size_t memory_usage = (size_t)abs(crop_set.x_min - crop_set.x_max) * (size_t)abs(crop_set.y_min - crop_set.y_max) * (size_t)nchannels * sizeof(uint16_t);

            if (memory_usage > mem_limit)
            {
                operation_t.output_height *= 0.9;
                operation_t.output_width *= 0.9;
                crop_set = choseCropArea(operation_t);
                // logger->critical("TOO MUCH MEMORY %llu", memory_usage);
                goto recheck_memory;
            }
        }

        int calculateSegmentNumberToSplitInto(WarpOperation &operation_t, double &median_dist)
        {
            int nsegs = 1;
            std::vector<satdump::projection::GCP> gcps_curr = operation_t.ground_control_points;
            // Filter GCPs, only keep each first y in x
            std::sort(gcps_curr.begin(), gcps_curr.end(),
                      [&operation_t](auto &el1, auto &el2)
                      {
                          return el1.y * operation_t.input_image->width() + el1.x < el2.y * operation_t.input_image->width() + el2.x;
                      });
            {
                std::vector<satdump::projection::GCP> gcps_curr_bkp = gcps_curr;
                gcps_curr.clear();
                for (int y = 0; y < (int)gcps_curr_bkp.size() - 1; y++)
                {
                    auto gcp1 = gcps_curr_bkp[y];
                    auto gcp2 = gcps_curr_bkp[y + 1];
                    if (gcp1.y != gcp2.y)
                        gcps_curr.push_back(gcp2);
                }
            }

            std::vector<double> distances;
            for (int y = 0; y < (int)gcps_curr.size() - 1; y++)
            {
                auto gcp1 = gcps_curr[y];
                auto gcp2 = gcps_curr[y + 1];

                auto gcp_dist = geodetic::vincentys_inverse(geodetic::geodetic_coords_t(gcp1.lat, gcp1.lon, 0), geodetic::geodetic_coords_t(gcp2.lat, gcp2.lon, 0));
                distances.push_back(gcp_dist.distance);
            }

            std::sort(distances.begin(), distances.end());

            double media_dist = median_dist = (distances.size() == 0 ? 0 : distances[distances.size() * 0.5]);

            nsegs = int((media_dist * gcps_curr.size()) / SEGMENT_SIZE_KM);
            if (nsegs == 0)
                nsegs = 1;

            logger->trace("We will split into %d segments. Median distance is %f km and total (avg) distance is %f km", nsegs, media_dist, media_dist * gcps_curr.size());

            return nsegs;
        }

        struct SegmentConfig
        {
            int y_start;
            int y_end;
            int shift_lon;
            int shift_lat;
            std::vector<satdump::projection::GCP> gcps;

            std::shared_ptr<projection::VizGeorefSpline2D> tps = nullptr;
        };

        void computeGCPCenter(std::vector<satdump::projection::GCP> &gcps, double &lon, double &lat)
        {
            double x_total = 0;
            double y_total = 0;
            double z_total = 0;

            for (auto &pt : gcps)
            {
                x_total += cos(pt.lat * DEG_TO_RAD) * cos(pt.lon * DEG_TO_RAD);
                y_total += cos(pt.lat * DEG_TO_RAD) * sin(pt.lon * DEG_TO_RAD);
                z_total += sin(pt.lat * DEG_TO_RAD);
            }

            x_total /= gcps.size();
            y_total /= gcps.size();
            z_total /= gcps.size();

            lon = atan2(y_total, x_total) * RAD_TO_DEG;
            double hyp = sqrt(x_total * x_total + y_total * y_total);
            lat = atan2(z_total, hyp) * RAD_TO_DEG;
        }

        void updateGCPOverlap(WarpOperation &operation_t, SegmentConfig &scfg, bool start_overlap, bool end_overlap)
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

            if (scfg.y_start < 0)
                scfg.y_start = 0;
            if (scfg.y_end > (int)operation_t.input_image->height())
                scfg.y_end = operation_t.input_image->height();
        }

        std::vector<satdump::projection::GCP> filter_gcps_position(std::vector<satdump::projection::GCP> gcps, double max_distance)
        {
            double center_lat = 0, center_lon = 0;
            computeGCPCenter(gcps, center_lon, center_lat);
            auto gcp_2 = gcps;
            gcps.clear();
            for (auto &gcp : gcp_2)
            {
                auto median_dis = geodetic::vincentys_inverse(geodetic::geodetic_coords_t(gcp.lat, gcp.lon, 0), geodetic::geodetic_coords_t(center_lat, center_lon, 0));
                if (median_dis.distance < max_distance)
                    gcps.push_back(gcp);
            }
            return gcps;
        }

        std::vector<SegmentConfig> prepareSegmentsAndSplitCuts(double nsegs, WarpOperation &operation_t, double median_dist)
        {
            std::vector<SegmentConfig> segmentConfigs;

            for (int segment = 0; segment < nsegs; segment++)
            {
                auto generateSeg = [&segmentConfigs, &operation_t](int start, int end, bool start_overlap, bool end_overlap)
                {
                    SegmentConfig scfg;

                    scfg.y_start = start;
                    scfg.y_end = end;

                    // Compute overlap if necessary
                    updateGCPOverlap(operation_t, scfg, start_overlap, end_overlap);

                    // Keep only GCPs for this segment
                    auto gcps_old = operation_t.ground_control_points;
                    for (auto gcp : gcps_old)
                    {
                        if (gcp.y >= scfg.y_start && gcp.y < scfg.y_end)
                        {
                            gcp.y -= scfg.y_start;
                            scfg.gcps.push_back(gcp);
                        }
                    }

                    // Filter obviously bogus GCPs
                    scfg.gcps = filter_gcps_position(scfg.gcps, SEGMENT_SIZE_KM * 2);

                    // Calculate center, and handle longitude shifting
                    {
                        double center_lat = 0, center_lon = 0;
                        computeGCPCenter(scfg.gcps, center_lon, center_lat);
                        scfg.shift_lon = -center_lon;
                        scfg.shift_lat = 0;
                    }

                    // Check for GCPs near the poles. If any is close, it means this segment needs to be handled as a pole!
                    for (auto gcp : scfg.gcps)
                    {
                        auto south_dis = geodetic::vincentys_inverse(geodetic::geodetic_coords_t(gcp.lat, gcp.lon, 0), geodetic::geodetic_coords_t(-90, 0, 0));
                        auto north_dis = geodetic::vincentys_inverse(geodetic::geodetic_coords_t(gcp.lat, gcp.lon, 0), geodetic::geodetic_coords_t(90, 0, 0));

                        if (south_dis.distance < MIN_POLE_DISTANCE)
                        {
                            scfg.shift_lon = 0;
                            scfg.shift_lat = -90;
                        }
                        if (north_dis.distance < MIN_POLE_DISTANCE)
                        {
                            scfg.shift_lon = 0;
                            scfg.shift_lat = 90;
                        }
                    }

                    segmentConfigs.push_back(scfg);
                };

                // Calculate start / end
                int y_start = (segment / nsegs) * operation_t.input_image->height();
                int y_end = ((segment + 1) / nsegs) * operation_t.input_image->height();

                // Isolate GCPs for this segment
                std::vector<satdump::projection::GCP> gcps_curr;
                for (auto gcp : operation_t.ground_control_points)
                {
                    if (gcp.y >= y_start && gcp.y < y_end)
                    {
                        // gcp.y -= y_start;
                        gcps_curr.push_back(gcp);
                    }
                }

                // Filter obviously bogus GCPs
                gcps_curr = filter_gcps_position(gcps_curr, SEGMENT_SIZE_KM * 2);

                // Filter GCPs, only keep each first y in x
                std::sort(gcps_curr.begin(), gcps_curr.end(),
                          [&operation_t](auto &el1, auto &el2)
                          {
                              return el1.y * operation_t.input_image->width() + el1.x < el2.y * operation_t.input_image->width() + el2.x;
                          });
                {
                    std::vector<satdump::projection::GCP> gcps_curr_bkp = gcps_curr;
                    gcps_curr.clear();
                    for (int y = 0; y < (int)gcps_curr_bkp.size() - 1 && gcps_curr_bkp.size() > 1; y++)
                    {
                        auto gcp1 = gcps_curr_bkp[y];
                        auto gcp2 = gcps_curr_bkp[y + 1];
                        if (gcp1.y != gcp2.y)
                            gcps_curr.push_back(gcp2);
                    }
                }

                // Check if this segment is cut (eg, los of signal? Different recorded dump?)
                std::vector<int> cutPositions;
                for (int y = 0; y < (int)gcps_curr.size() - 1 && gcps_curr.size() > 1; y++)
                    if (geodetic::vincentys_inverse(geodetic::geodetic_coords_t(gcps_curr[y].lat, gcps_curr[y].lon, 0),
                                                    geodetic::geodetic_coords_t(gcps_curr[y + 1].lat, gcps_curr[y + 1].lon, 0))
                            .distance > (8 * median_dist))
                        cutPositions.push_back(gcps_curr[y + 1].y);

                // Generate, handling cuts
                if (cutPositions.size() > 0)
                {
                    generateSeg(y_start, cutPositions[0], true, false);
                    for (size_t i = 1; i < cutPositions.size() - 1; i++)
                        generateSeg(cutPositions[i], y_end, false, false);
                    generateSeg(cutPositions[cutPositions.size() - 1], y_end, false, true);
                }
                else
                {
                    generateSeg(y_start, y_end, true, true);
                }
            }

            return segmentConfigs;
        }

        WarpResult performSmartWarp(WarpOperation operation_t, float *progress)
        {
            if (operation_t.input_image->size() == 0)
                throw satdump_exception("Can't warp an empty image!");

            WarpResult result; // Final output

            // Prepare crop area, and check it can fit in RAM
            satdump::warp::WarpCropSettings crop_set = choseCropArea(operation_t);
            int nchannels = operation_t.output_rgba ? 4 : operation_t.input_image->channels();

            ensureMemoryLimit(crop_set, operation_t, nchannels, MAX_IMAGE_RAM_USAGE);

            logger->trace("Smart Warp using %d %d, full size %d %d", crop_set.x_max - crop_set.x_min, crop_set.y_max - crop_set.y_min, operation_t.output_width, operation_t.output_height);

            // Prepare the output
            result.output_image = image::Image(16, // TODOIMG not just 8-bits
                                                crop_set.x_max - crop_set.x_min, crop_set.y_max - crop_set.y_min,
                                                nchannels);
            result.top_left = {0, 0, (double)crop_set.lon_min, (double)crop_set.lat_max};                                                                                  // 0,0
            result.top_right = {(double)result.output_image.width() - 1, 0, (double)crop_set.lon_max, (double)crop_set.lat_max};                                           // 1,0
            result.bottom_left = {0, (double)result.output_image.height() - 1, (double)crop_set.lon_min, (double)crop_set.lat_min};                                        // 0,1
            result.bottom_right = {(double)result.output_image.width() - 1, (double)result.output_image.height() - 1, (double)crop_set.lon_max, (double)crop_set.lat_min}; // 1,1

            // Prepare projection to draw segments
            geodetic::projection::EquirectangularProjection projector_final;
            projector_final.init(result.output_image.width(), result.output_image.height(), result.top_left.lon, result.top_left.lat, result.bottom_right.lon, result.bottom_right.lat);

            /// Try to calculate the number of segments to split the data into. All an approximation, but it's good enough!
            double median_dist = 0;
            int nsegs = calculateSegmentNumberToSplitInto(operation_t, median_dist);

            // Generate all segments
            std::vector<SegmentConfig> segmentConfigs = prepareSegmentsAndSplitCuts(nsegs, operation_t, median_dist);

#pragma omp parallel for
            //  Solve all TPS transforms, multithreaded
            for (int64_t ns = 0; ns < (int64_t)segmentConfigs.size(); ns++)
                segmentConfigs[ns].tps = initTPSTransform(segmentConfigs[ns].gcps, segmentConfigs[ns].shift_lon, segmentConfigs[ns].shift_lat);

            int scnt = 0;
            // Process all the segments
            for (auto &segmentCfg : segmentConfigs)
            {
                // Copy operation for the segment Warp
                image::Image segment_image = operation_t.input_image->crop_to(0, segmentCfg.y_start, operation_t.input_image->width(), segmentCfg.y_end);
                auto operation = operation_t;
                operation.input_image = &segment_image;
                operation.shift_lon = segmentCfg.shift_lon;
                operation.shift_lat = segmentCfg.shift_lat;
                operation.ground_control_points = segmentCfg.gcps;

                // Perform the actual warp
                satdump::warp::ImageWarper warper;
                warper.op = operation;
                warper.set_tps(segmentCfg.tps);
                warper.update(true);

                satdump::warp::WarpResult result2 = warper.warp();

#if 0 // Draw GCPs, useful for debug
      // Setup projector....
                geodetic::projection::EquirectangularProjection projector;
                projector.init(result2.output_image.width(), result2.output_image.height(),
                               result2.top_left.lon, result2.top_left.lat,
                               result2.bottom_right.lon, result2.bottom_right.lat);

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

                        result2.output_image.draw_circle(pos.first, pos.second, 5, color, true);
                    }
                }
#endif

                // .....and re-project! (just a basic affine transform)
                float lon = result2.top_left.lon, lat = result2.top_left.lat;
                int x2 = 0, y2 = 0;
                projector_final.forward(lon, lat, x2, y2, true);
                // if (!(x2 == -1 || y2 == -1))
                {
                    // Slightly modified draw_image()
                    int width = std::min<int>(result.output_image.width(), x2 + result2.output_image.width()) - x2;
                    int height = std::min<int>(result.output_image.height(), y2 + result2.output_image.height()) - y2;

                    if (result2.output_image.channels() == result.output_image.channels())
#pragma omp parallel for
                        for (int x = 0; x < width; x++)
                            for (int y = 0; y < height; y++)
                                if (y + y2 >= 0 && x + x2 >= 0)
                                    if (result2.output_image.get(3, y * result2.output_image.width() + x) > 0)
                                    {
                                        for (int ch = 0; ch < 3; ch++)
                                            result.output_image.set(ch, (y + y2) * result.output_image.width() + x + x2, result2.output_image.get(ch, y * result2.output_image.width() + x));

                                        if (result2.output_image.channels() == 4)
                                            result.output_image.set(3, (y + y2) * result.output_image.width() + x + x2, result2.output_image.get(3, y * result2.output_image.width() + x));
                                        else
                                            result.output_image.set(3, (y + y2) * result.output_image.width() + x + x2, 65535);
                                    }
                }

                scnt++;
                if (progress != nullptr)
                    *progress = (float)scnt / (float)segmentConfigs.size();

                /////////// DEBUG
                // result2.output_image.save_img("projtest/test" + std::to_string((scnt) + 1));
            }

            return result;
        }
    }
}
