#include "timestamp_line_gcps.h"
#include "nlohmann/json_utils.h"
#include "common/geodetic/vincentys_calculations.h"
#include "core/exception.h"

namespace satdump
{
    namespace projection
    {
        inline double lon_shift(double lon, double shift)
        {
            if (shift == 0)
                return lon;
            lon += shift;
            if (lon > 180)
                lon -= 360;
            if (lon < -180)
                lon += 360;
            return lon;
        }

        inline void shift_latlon_by_lat(double *lat, double *lon, double latshift)
        {
            if (latshift == 0)
                return;

            double x = cos(*lat * DEG_TO_RAD) * cos(*lon * DEG_TO_RAD);
            double y = cos(*lat * DEG_TO_RAD) * sin(*lon * DEG_TO_RAD);
            double z = sin(*lat * DEG_TO_RAD);

            double theta = latshift * DEG_TO_RAD;

            double x2 = x * cos(theta) + z * sin(theta);
            double y2 = y;
            double z2 = z * cos(theta) - x * sin(theta);

            *lon = atan2(y2, x2) * RAD_TO_DEG;
            double hyp = sqrt(x2 * x2 + y2 * y2);
            *lat = atan2(z2, hyp) * RAD_TO_DEG;
        }

        TimestampLineGCPsRaytracer::TimestampLineGCPsRaytracer(nlohmann::json cfg)
            : SatelliteRaytracer(cfg)
        {
            timestamps = cfg["timestamps"].get<std::vector<double>>();

            int gcps_cnt = cfg["gcp_cnt"];
            for (int i = 0; i < gcps_cnt; i++)
            {
                int x = cfg["gcps"][i]["x"];
                int y = cfg["gcps"][i]["y"];
                double lat = cfg["gcps"][i]["lat"];
                double lon = cfg["gcps"][i]["lon"];

                while (y >= gcps.size())
                    gcps.push_back(GCPLine());

                gcps[y].g.push_back({x, y, lat, lon});
            }

            for (auto &s : gcps)
            {
                // Sort them by X
                std::sort(s.g.begin(), s.g.end(),
                          [](auto &el1, auto &el2)
                          { return el1.x < el2.x; });

                // Calculate their center Lat
                {
                    double x_total = 0;
                    double y_total = 0;
                    double z_total = 0;
                    for (auto &g : s.g)
                    {
                        x_total += cos(g.lat * DEG_TO_RAD) * cos(g.lon * DEG_TO_RAD);
                        y_total += cos(g.lat * DEG_TO_RAD) * sin(g.lon * DEG_TO_RAD);
                        z_total += sin(g.lat * DEG_TO_RAD);
                    }

                    x_total /= s.g.size();
                    y_total /= s.g.size();
                    z_total /= s.g.size();

                    double hyp = sqrt(x_total * x_total + y_total * y_total);
                    s.clat = atan2(z_total, hyp) * RAD_TO_DEG;
                }

                // Shift them all towards 0, 0
                for (auto &g : s.g)
                    shift_latlon_by_lat(&g.lat, &g.lon, -s.clat);

                // Calculate their center Lon,
                {
                    double x_total = 0;
                    double y_total = 0;
                    double z_total = 0;
                    for (auto &g : s.g)
                    {
                        x_total += cos(g.lat * DEG_TO_RAD) * cos(g.lon * DEG_TO_RAD);
                        y_total += cos(g.lat * DEG_TO_RAD) * sin(g.lon * DEG_TO_RAD);
                        z_total += sin(g.lat * DEG_TO_RAD);
                    }

                    x_total /= s.g.size();
                    y_total /= s.g.size();
                    z_total /= s.g.size();

                    s.clon = atan2(y_total, x_total) * RAD_TO_DEG;
                }

                for (auto &g : s.g)
                    g.lon = lon_shift(g.lon, -s.clon);

                if (s.g.size() < 2)
                    throw satdump_exception("Needs at LEAST 2 GCPS per line!");
            }
        }

        bool TimestampLineGCPsRaytracer::get_position(double x, double y, geodetic::geodetic_coords_t &pos, double *otime)
        {
            if (y >= gcps.size() || y < 0)
                return 1;

            int iy = floor(y);

            if (y < timestamps.size() && y > 0)
            {
                double timestamp = timestamps[iy];
                if (otime != nullptr)
                    *otime = timestamp;
            }

            auto &l = gcps[y];

            int start_pos = 0;
            while (start_pos < (int)l.g.size() && x > l.g[start_pos].x)
                start_pos++;

            if (start_pos + 1 == (int)l.g.size())
                start_pos--;

            double x1 = l.g[start_pos].x;
            double x2 = l.g[start_pos + 1].x;

            double x_len = x2 - x1;
            x -= x1;

            // Lat
            {
                double y1 = l.g[start_pos].lat;
                double y2 = l.g[start_pos + 1].lat;

                double y_len = y2 - y1;
                pos.lat = y1 + (x / x_len) * y_len;
            }

            // Lon
            {
                double y1 = l.g[start_pos].lon;
                double y2 = l.g[start_pos + 1].lon;

                double y_len = y2 - y1;
                pos.lon = y1 + (x / x_len) * y_len;
            }

            pos.lon = lon_shift(pos.lon, l.clon);
            shift_latlon_by_lat(&pos.lat, &pos.lon, l.clat);

            return 0;
        }
    }
}