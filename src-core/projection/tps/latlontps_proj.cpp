#include "latlontps_proj.h"
#include "core/exception.h"
#include "common/geodetic/vincentys_calculations.h"
#include <map>
#include "logger.h"

namespace satdump
{
    namespace warp // TODOREWORK REWRITE / MOVE OUT
    {
        double lon_shift(double lon, double shift);
        void shift_latlon_by_lat(double *lat, double *lon, double shift);

        void computeGCPCenter(std::vector<satdump::projection::GCP> &gcps, double &lon, double &lat);
    }

    namespace proj
    {
        namespace
        {
            std::shared_ptr<projection::VizGeorefSpline2D> initTPSTransformFWD(std::vector<projection::GCP> gcps, int shift_lon, int shift_lat)
            {
                std::shared_ptr<projection::VizGeorefSpline2D> spline_transform = std::make_shared<projection::VizGeorefSpline2D>(2);

                // Attach (non-redundant) points to the transformation.
                std::map<std::pair<double, double>, int> oMapPixelLineToIdx;
                std::map<std::pair<double, double>, int> oMapXYToIdx;
                for (int iGCP = 0; iGCP < (int)gcps.size(); iGCP++)
                {
                    double final_lon = warp::lon_shift(gcps[iGCP].lon, shift_lon);
                    double final_lat = gcps[iGCP].lat;

                    warp::shift_latlon_by_lat(&final_lat, &final_lon, shift_lat);

                    const double afPL[2] = {gcps[iGCP].x, gcps[iGCP].y};
                    const double afXY[2] = {final_lon, final_lat};

                    std::map<std::pair<double, double>, int>::iterator oIter(oMapPixelLineToIdx.find(std::pair<double, double>(afPL[0], afPL[1])));

                    if (oIter != oMapPixelLineToIdx.end())
                    {
                        if (afXY[0] == gcps[oIter->second].lon && afXY[1] == gcps[oIter->second].lat)
                            continue;
                        else
                        {
                            logger->warn("2 GCPs have the same X,Y!");
                            continue;
                        }
                    }
                    else
                        oMapPixelLineToIdx[std::pair<double, double>(afPL[0], afPL[1])] = iGCP;

                    if (oMapXYToIdx.find(std::pair<double, double>(afXY[0], afXY[1])) != oMapXYToIdx.end())
                    {
                        logger->warn("2 GCPs have the same Lat,Lon!");
                        continue;
                    }
                    else
                        oMapXYToIdx[std::pair<double, double>(afXY[0], afXY[1])] = iGCP;

                    if (!spline_transform->add_point(afXY[0], afXY[1], afPL))
                    {
                        logger->error("Error generating transformer!");
                        // return 1;
                    }
                }

                logger->info("Solving TPS equations for %d GCPs...", gcps.size());
                auto solve_start = std::chrono::system_clock::now();
                bool solved = spline_transform->solve() != 0;
                if (solved)
                    logger->info("Solved! Took %f", (std::chrono::system_clock::now() - solve_start).count() / 1e9);
                else
                    logger->error("Failure solving!");

                return spline_transform;
            }

            std::shared_ptr<projection::VizGeorefSpline2D> initTPSTransformREV(std::vector<projection::GCP> gcps, int shift_lon, int shift_lat)
            {
                std::shared_ptr<projection::VizGeorefSpline2D> spline_transform = std::make_shared<projection::VizGeorefSpline2D>(2);

                // Attach (non-redundant) points to the transformation.
                std::map<std::pair<double, double>, int> oMapPixelLineToIdx;
                std::map<std::pair<double, double>, int> oMapXYToIdx;
                for (int iGCP = 0; iGCP < (int)gcps.size(); iGCP++)
                {
                    double final_lon = warp::lon_shift(gcps[iGCP].lon, shift_lon);
                    double final_lat = gcps[iGCP].lat;

                    warp::shift_latlon_by_lat(&final_lat, &final_lon, shift_lat);

                    const double afPL[2] = {gcps[iGCP].x, gcps[iGCP].y};
                    const double afXY[2] = {final_lon, final_lat};

                    std::map<std::pair<double, double>, int>::iterator oIter(oMapPixelLineToIdx.find(std::pair<double, double>(afPL[0], afPL[1])));

                    if (oIter != oMapPixelLineToIdx.end())
                    {
                        if (afXY[0] == gcps[oIter->second].lon && afXY[1] == gcps[oIter->second].lat)
                            continue;
                        else
                        {
                            logger->warn("2 GCPs have the same X,Y!");
                            continue;
                        }
                    }
                    else
                        oMapPixelLineToIdx[std::pair<double, double>(afPL[0], afPL[1])] = iGCP;

                    if (oMapXYToIdx.find(std::pair<double, double>(afXY[0], afXY[1])) != oMapXYToIdx.end())
                    {
                        logger->warn("2 GCPs have the same Lat,Lon!");
                        continue;
                    }
                    else
                        oMapXYToIdx[std::pair<double, double>(afXY[0], afXY[1])] = iGCP;

                    if (!spline_transform->add_point(afPL[0], afPL[1], afXY))
                    {
                        logger->error("Error generating transformer!");
                        // return 1;
                    }
                }

                logger->info("Solving TPS equations for %d GCPs...", gcps.size());
                auto solve_start = std::chrono::system_clock::now();
                bool solved = spline_transform->solve() != 0;
                if (solved)
                    logger->info("Solved! Took %f", (std::chrono::system_clock::now() - solve_start).count() / 1e9);
                else
                    logger->error("Failure solving!");

                return spline_transform;
            }
        }

        LatLonTpsProjHelper::LatLonTpsProjHelper(std::vector<satdump::projection::GCP> gcps, bool fwd, bool rev)
        { // Calculate center, and handle longitude shifting
            {
                double center_lat = 0, center_lon = 0;
                warp::computeGCPCenter(gcps, center_lon, center_lat);
                shift_lon = -center_lon;
                shift_lat = 0;
            }

            // Check for GCPs near the poles. If any is close, it means this segment needs to be handled as a pole!
            for (auto &gcp : gcps)
            {
                auto south_dis = geodetic::vincentys_inverse(geodetic::geodetic_coords_t(gcp.lat, gcp.lon, 0), geodetic::geodetic_coords_t(-90, 0, 0));
                auto north_dis = geodetic::vincentys_inverse(geodetic::geodetic_coords_t(gcp.lat, gcp.lon, 0), geodetic::geodetic_coords_t(90, 0, 0));
#define MIN_POLE_DISTANCE 1000 // Maximum distance at which to consider we're working over a pole
                if (south_dis.distance < MIN_POLE_DISTANCE)
                {
                    shift_lon = 0;
                    shift_lat = -90;
                }
                if (north_dis.distance < MIN_POLE_DISTANCE)
                {
                    shift_lon = 0;
                    shift_lat = 90;
                }
            }

            if (fwd)
                tps_fwd = initTPSTransformFWD(gcps, shift_lon, shift_lat);

            if (rev)
                tps_rev = initTPSTransformREV(gcps, shift_lon, shift_lat);
        }

        void LatLonTpsProjHelper::forward(geodetic::geodetic_coords_t pos, double &x, double &y)
        {
            warp::shift_latlon_by_lat(&pos.lat, &pos.lon, shift_lat);
            tps_fwd->get_point(warp::lon_shift(pos.lon, shift_lon), pos.lat, tps_fwd_xy);
            x = tps_fwd_xy[0];
            y = tps_fwd_xy[1];
        }

        void LatLonTpsProjHelper::reverse(double x, double y, geodetic::geodetic_coords_t &pos)
        {
            tps_rev->get_point(x, y, tps_rev_ll);
            pos.lat = tps_rev_ll[0];
            pos.lon = warp::lon_shift(tps_rev_ll[1], -shift_lon);
            warp::shift_latlon_by_lat(&pos.lat, &pos.lon, -shift_lat);
        }
    }
}