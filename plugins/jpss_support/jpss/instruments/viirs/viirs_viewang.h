#pragma once

#include <vector>

namespace jpss
{
    namespace viirs
    {
        std::vector<std::pair<double, double>> calculateVIIRSViewAnglePoints(bool is_dnb, bool is_noaa20, double scan_angle, double roll_offset)
        {
            std::vector<std::pair<double, double>> points;

            if (is_dnb)
            {
                std::vector<int> aggr_width = {80, 16, 64, 64, 64, 32, 24, 72, 40, 56, 40, 48, 32, 48, 32, 72, 72, 72, 80, 56, 80, 64, 64, 64, 64, 64, 72, 80, 72, 88, 72, 184,
                                               184, 72, 88, 72, 80, 72, 64, 64, 64, 64, 64, 80, 56, 80, 72, 72, 72, 32, 48, 32, 48, 40, 56, 40, 72, 24, 32, 64, 64, 64, 16, 80};
                std::vector<int> aggr_inter = {11, 12, 12, 13, 14, 15, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 26, 28, 30, 33, 35, 38, 40, 43, 46, 49, 52, 55, 59, 62, 64, 66};

                std::vector<int> aggr_mode_normal = {32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1,
                                                     1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32};

                std::vector<int> aggr_mode_jpss1 = {21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1,
                                                    2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 32};

                if (is_noaa20)
                {
                    aggr_width = {736, 32, 48, 32, 72, 72, 72, 80, 56, 80, 64, 64, 64, 64, 64, 72, 80, 72, 88, 72,
                                  368, 72, 88, 72, 80, 72, 64, 64, 64, 64, 64, 80, 56, 80, 72, 72, 72, 32, 48, 32, 456, 8};
                }

                std::vector<int> vals_to_cover;
                vals_to_cover.push_back(0);

                auto &aggr_mode_c = is_noaa20 ? aggr_mode_jpss1 : aggr_mode_normal;

                int all_px = 0, all_px2 = 0;
                for (size_t zone = 0; zone < aggr_mode_c.size(); zone++)
                {
                    vals_to_cover.push_back(4063 - all_px2);
                    all_px += aggr_width[zone] * aggr_inter[31 - (aggr_mode_c[zone] - 1)];
                    all_px2 += aggr_width[zone];
                }
                // logger->critical("DNB AGGR %d %d", all_px, all_px2);

                vals_to_cover.push_back(4064);

                double final_width = is_noaa20 ? all_px : 145040;

                for (int x : vals_to_cover)
                {
                    int final_x = 4063 - x;
                    int current_zoff = 0;
                    int current_zoff2 = 0;
                    int current_zone = 0;
                    while (current_zone < (int)aggr_mode_c.size())
                    {
                        if ((current_zoff + aggr_width[current_zone]) >= final_x)
                            break;

                        current_zoff += aggr_width[current_zone];
                        current_zoff2 += aggr_width[current_zone] * aggr_inter[31 - (aggr_mode_c[current_zone] - 1)];
                        current_zone++;
                    }

                    double x2 = current_zoff2 + (final_x - current_zoff) * aggr_inter[31 - (aggr_mode_c[current_zone] - 1)];
                    x2 = -((x2 - (final_width / 2.0)) / final_width) * scan_angle + roll_offset;
                    points.push_back({x, x2});
                }
            }
            else
            {
                const std::vector<int> aggr_width = {1280, 736, 1184, 1184, 736, 1280};
                const std::vector<int> aggr_inter = {1, 2, 3, 3, 2, 1};

                const double final_width = 12608;

                std::vector<int> vals_to_cover = {0, 1279, 2015,
                                                  3199,
                                                  4383, 5119, 6399};

                for (int x : vals_to_cover)
                {
                    int current_zoff = 0;
                    int current_zoff2 = 0;
                    int current_zone = 0;
                    while (current_zone < 6)
                    {
                        if ((current_zoff + aggr_width[current_zone]) >= x)
                            break;

                        current_zoff += aggr_width[current_zone];
                        current_zoff2 += aggr_width[current_zone] * aggr_inter[current_zone];
                        current_zone++;
                    }

                    double x2 = current_zoff2 + (x - current_zoff) * aggr_inter[current_zone];
                    x2 = ((x2 - (final_width / 2.0)) / final_width) * scan_angle + roll_offset;
                    points.push_back({x, x2});
                }
            }

            return points;
        };
    }
}