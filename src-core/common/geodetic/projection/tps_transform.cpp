#include "tps_transform.h"
#include <map>
#include "logger.h"
#include <thread>

/*
 This file is originally from https://github.com/OSGeo/gdal
 It was modified for the purposes required here.

 All credits go to the GDAL Project
*/

namespace geodetic
{
    namespace projection
    {
        TPSTransform::TPSTransform()
        {
        }

        TPSTransform::TPSTransform(std::vector<GCP> gcps)
        {
            init(gcps);
        }

        int TPSTransform::init(std::vector<GCP> gcps)
        {
            // Allocate transform info.
            if (has_been_init)
            {
                delete spline_reverse;
                delete spline_forward;
            }
            spline_reverse = new VizGeorefSpline2D(2);
            spline_forward = new VizGeorefSpline2D(2);
            has_been_init = true;

            // Attach (non-redundant) points to the transformation.
            std::map<std::pair<double, double>, int> oMapPixelLineToIdx;
            std::map<std::pair<double, double>, int> oMapXYToIdx;
            for (int iGCP = 0; iGCP < (int)gcps.size(); iGCP++)
            {
                const double afPL[2] = {gcps[iGCP].x, gcps[iGCP].y};
                const double afXY[2] = {gcps[iGCP].lon, gcps[iGCP].lat};

                std::map<std::pair<double, double>, int>::iterator oIter(oMapPixelLineToIdx.find(std::pair<double, double>(afPL[0], afPL[1])));

                if (oIter != oMapPixelLineToIdx.end())
                {
                    if (afXY[0] == gcps[oIter->second].lon && afXY[1] == gcps[oIter->second].lat)
                    {
                        continue;
                    }
                    else
                    {
                        logger->warn("2 GCPs have the same X,Y!");
                    }
                }
                else
                {
                    oMapPixelLineToIdx[std::pair<double, double>(afPL[0], afPL[1])] = iGCP;
                }

                oIter = oMapXYToIdx.find(std::pair<double, double>(afXY[0], afXY[1]));
                if (oIter != oMapXYToIdx.end())
                {
                    logger->warn("2 GCPs have the same Lat,Lon!");
                }
                else
                {
                    oMapXYToIdx[std::pair<double, double>(afXY[0], afXY[1])] = iGCP;
                }

                if (!(spline_reverse->add_point(afPL[0], afPL[1], afXY) && spline_forward->add_point(afXY[0], afXY[1], afPL)))
                {
                    logger->error("Error generating transformer!");
                    return 1;
                }
            }

            // Solve forward and reverse
            logger->info("Solving TPS equations...");
            std::thread solveFwd([this]()
                                 {
                                     fwd_solved = spline_forward->solve() != 0;
                                     logger->info("Forward solved");
                                 });
            rev_solved = spline_reverse->solve() != 0;
            logger->info("Reverse solved");
            if (solveFwd.joinable())
                solveFwd.join();

            if (!fwd_solved || !rev_solved)
            {
                logger->error("Error generating transformer!");
                return 1;
            }

            return 0;
        }

        TPSTransform::~TPSTransform()
        {
            if (has_been_init)
            {
                delete spline_reverse;
                delete spline_forward;
            }
        }

        void TPSTransform::forward(double lon, double lat, double &x, double &y)
        {
            spline_forward->get_point(lon, lat, xy);
            x = xy[0];
            y = xy[1];
        }

        void TPSTransform::inverse(double x, double y, double &lon, double &lat)
        {
            spline_reverse->get_point(x, y, xy);
            lon = xy[0];
            lat = xy[1];
        }

    };
};