/**********************************************************************
 * This file is used for testing random stuff without running the
 * whole of SatDump, which comes in handy for debugging individual
 * elements before putting them all together in modules...
 *
 * If you are an user, ignore this file which will not be built by
 * default, and if you're a developper in need of doing stuff here...
 * Go ahead!
 *
 * Don't judge the code you might see in there! :)
 **********************************************************************/

#include "logger.h"

#include <fstream>
#include "common/utils.h"

#include <cmath>
#include "common/geodetic/wgs84.h"
#include "common/geodetic/geodetic_coordinates.h"

#include "common/image/image.h"

#include "libs/predict/predict.h"

#include "common/geodetic/ecef_to_eci.h"

#include <unistd.h>

int fcs(uint8_t *data, int len)
{
    int i;
    unsigned char c0 = 0;
    unsigned char c1 = 0;
    for (i = 0; i < len; i++)
    {
        c0 = c0 + *(data + i);
        c1 = c1 + c0;
    }
    return ((long)(c0 + c1)); /* returns zero if buffer is error-free*/
}

extern "C"
{
    void observer_calculate(const predict_observer_t *observer, double time, const double pos[3], const double vel[3], struct predict_observation *result);
}

namespace
{
    // Define GPS leap seconds
    uint64_t leap_seconds[] = {46828800, 78364801, 109900802, 173059203, 252028804, 315187205, 346723206, 393984007, 425520008, 457056009, 504489610, 551750411, 599184012, 820108813, 914803214, 1025136015, 1119744016, 1167264017};
    int leapLen = 18;

    // Test to see if a GPS second is a leap second
    bool isleap(uint64_t gpsTime)
    {
        bool isLeap = false;
        for (int i = 0; i < leapLen; i++)
            if (gpsTime == leap_seconds[i])
                isLeap = true;
        return isLeap;
    }

    // Count number of leap seconds that have passed
    int countleaps(uint64_t gpsTime, bool to_gps)
    {
        int nleaps = 0; // number of leap seconds prior to gpsTime
        for (int i = 0; i < leapLen; i++)
        {
            if (!to_gps)
            {
                if (gpsTime >= leap_seconds[i] - i)
                    nleaps++;
            }
            else if (to_gps)
            {
                if (gpsTime >= leap_seconds[i])
                    nleaps++;
            }
        }
        return nleaps;
    }

    // Convert GPS Time to Unix Time
    time_t gps2unix(uint64_t gpsTime)
    {
        // Add offset in seconds
        time_t unixTime = gpsTime + 315964800;
        int nleaps = countleaps(gpsTime, false);
        unixTime = unixTime - nleaps;
        if (isleap(gpsTime))
            unixTime = unixTime + 0.5;
        return unixTime;
    }

    time_t gps_time_to_unix(uint64_t gps_weeks, uint64_t gps_week_time)
    {
        return gps2unix(gps_weeks * 604800 + gps_week_time);
    }

}

namespace
{
    struct vector
    {
        double x;
        double y;
        double z;
    };

    // Must already be in radians!
    void lla2xyz(geodetic::geodetic_coords_t lla, vector &position)
    {
        // double asq = geodetic::WGS84::a * geodetic::WGS84::a;
        double esq = geodetic::WGS84::e * geodetic::WGS84::e;
        double N = geodetic::WGS84::a / sqrt(1 - esq * pow(sin(lla.lat), 2));
        position.x = (N + lla.alt) * cos(lla.lat) * cos(lla.lon);
        position.y = (N + lla.alt) * cos(lla.lat) * sin(lla.lon);
        position.z = ((1 - esq) * N + lla.alt) * sin(lla.lat);
    }

    void xyz2lla(vector position, geodetic::geodetic_coords_t &lla)
    {
        double asq = geodetic::WGS84::a * geodetic::WGS84::a;
        double esq = geodetic::WGS84::e * geodetic::WGS84::e;

        double b = sqrt(asq * (1 - esq));
        double bsq = b * b;
        double ep = sqrt((asq - bsq) / bsq);
        double p = sqrt(position.x * position.x + position.y * position.y);
        double th = atan2(geodetic::WGS84::a * position.z, b * p);
        double lon = atan2(position.y, position.x);
        double lat = atan2((position.z + ep * ep * b * pow(sin(th), 3)), (p - esq * geodetic::WGS84::a * pow(cos(th), 3)));
        // double N = geodetic::WGS84::a / (sqrt(1 - esq * pow(sin(lat), 2)));

        vector g;
        lla2xyz(geodetic::geodetic_coords_t(lat, lon, 0, true), g);

        double gm = sqrt(g.x * g.x + g.y * g.y + g.z * g.z);
        double am = sqrt(position.x * position.x + position.y * position.y + position.z * position.z);
        double alt = am - gm;

        lla = geodetic::geodetic_coords_t(lat, lon, alt, true);
    }

    inline float az_el_to_plot_x(float plot_size, float radius, float az, float el) { return sin(az * DEG_TO_RAD) * plot_size * radius * ((90.0 - el) / 90.0); }
    inline float az_el_to_plot_y(float plot_size, float radius, float az, float el) { return cos(az * DEG_TO_RAD) * plot_size * radius * ((90.0 - el) / 90.0); }
}

#include "common/repack.h"

double calcFreq(int f, bool small = true)
{
    if (small)
    {
        if (f <= 0x40)
            f = 0x1 << 8 | f;
        else if (f >= 0x50)
            f = 0x0 << 8 | f;
    }
    return 137.0 + double(f) * 0.0025;
}

struct OrbComEphem
{
    time_t time;
    int scid;
    float az, el;
};

int main(int argc, char *argv[])
{
    initLogger();
    completeLoggerInit();

    std::ifstream data_in(argv[1], std::ios::binary);

    uint8_t frm[600];

    vector observer_pos;
    lla2xyz({48.0 * DEG_TO_RAD, 1.0 * DEG_TO_RAD, 173.0 / 1000.0, true}, observer_pos);
    predict_observer_t *predict_obs = predict_create_observer("Main", 48.0 * DEG_TO_RAD, 1.0 * DEG_TO_RAD, 173.0);

    int plot_size = 512;
    image::Image<uint8_t> img(plot_size, plot_size, 3);
    // {
    // All black bg
    img.fill(0);

    // Draw the "target-like" plot with elevation rings
    float radius = 0.45;
    float radius1 = plot_size * radius * (3.0 / 9.0);
    float radius2 = plot_size * radius * (6.0 / 9.0);
    float radius3 = plot_size * radius * (9.0 / 9.0);

    uint8_t color_green[] = {0, 255, 0};
    uint8_t color_red[] = {255, 0, 0};
    uint8_t color_orange[] = {255, 165, 0};
    uint8_t color_cyan[] = {0, 237, 255};

    img.draw_circle(plot_size / 2, plot_size / 2,
                    radius1, color_green, false);
    img.draw_circle(plot_size / 2, plot_size / 2,
                    radius2, color_green, false);
    img.draw_circle(plot_size / 2, plot_size / 2,
                    radius3, color_green, false);

    img.draw_line(plot_size / 2, 0,
                  plot_size / 2, plot_size - 1,
                  color_green);
    img.draw_line(0, plot_size / 2,
                  plot_size - 1, plot_size / 2,
                  color_green);
    // }

    std::vector<OrbComEphem> all_ephem_points;

    while (!data_in.eof())
    {
        data_in.read((char *)frm, 600);

        for (int i = 0; i < 50; i++)
        {
            if (frm[i * 12] == 0x1F)
            {
                if (fcs(&frm[i * 12], 24) == 0)
                {
                    std::reverse(&frm[i * 12 + 2], &frm[i * 12 + 22]);

                    int scid = frm[i * 12 + 1];
                    int week_number = frm[i * 12 + 2] << 8 | frm[i * 12 + 3];
                    int time_of_week = frm[i * 12 + 4] << 16 | frm[i * 12 + 5] << 8 | frm[i * 12 + 6];

                    // logger->info("Week Number %d, Time Of Week %d", week_number, time_of_week);

                    const double MAX_R_SAT = 8378155.0;
                    const double VAL_20_BITS = 1048576.0;
                    const double MAX_V_SAT = 7700.0;

                    uint32_t values[0];
                    repackBytesTo20bits(&frm[i * 12 + 7], 15, values);

                    long x_raw = values[5];
                    long y_raw = values[4];
                    long z_raw = values[3];

                    long x_raw2 = values[2];
                    long y_raw2 = values[1];
                    long z_raw2 = values[0];

                    double X = ((2.0 * x_raw * MAX_R_SAT) / VAL_20_BITS - MAX_R_SAT) / 1000.0;
                    double Y = ((2.0 * y_raw * MAX_R_SAT) / VAL_20_BITS - MAX_R_SAT) / 1000.0;
                    double Z = ((2.0 * z_raw * MAX_R_SAT) / VAL_20_BITS - MAX_R_SAT) / 1000.0;

                    double X_DOT = ((2.0 * x_raw2 * MAX_V_SAT) / VAL_20_BITS - MAX_R_SAT) / 1000.0;
                    double Y_DOT = ((2.0 * y_raw2 * MAX_V_SAT) / VAL_20_BITS - MAX_R_SAT) / 1000.0;
                    double Z_DOT = ((2.0 * z_raw2 * MAX_V_SAT) / VAL_20_BITS - MAX_R_SAT) / 1000.0;

                    geodetic::geodetic_coords_t lla;
                    xyz2lla({X, Y, Z}, lla);
                    lla.toDegs();

                    // Az/El
                    double az, el, range, range_rate;
                    {
                        double pos[3];
                        pos[0] = X * 1000.0;
                        pos[1] = Y * 1000.0;
                        pos[2] = Z * 1000.0;

                        double vel[3];
                        vel[0] = X_DOT * 1000.0;
                        vel[1] = Y_DOT * 1000.0;
                        vel[2] = Z_DOT * 1000.0;

                        ecef_epehem_to_eci(0, pos[0], pos[1], pos[2], vel[0], vel[1], vel[2]);

                        predict_observation observ;
                        observer_calculate(predict_obs, predict_to_julian(0) + 2444238.5, pos, vel, &observ);

                        az = observ.azimuth * RAD_TO_DEG;
                        el = observ.elevation * RAD_TO_DEG;
                        range = observ.range;
                    }

                    time_t ephem_time = gps_time_to_unix(week_number, time_of_week);

                    logger->info("SCID %d, Week Number %d, Time Of Week %d, Time %s - X %.3d Y %.3d Z %.3d - X %.3f Y %.3f Z %.3f - Lon %.1f, Lat %.1f, Alt %.1f - Az %.1f El %.1f Range %.1f",
                                 scid + 70, week_number, time_of_week, timestamp_to_string(ephem_time).c_str(),
                                 x_raw, y_raw, z_raw,
                                 X_DOT, Y_DOT, Z_DOT,
                                 lla.lon, lla.lat, lla.alt,
                                 az, el, range);

                    // Polar Plot!
                    // Draw the current satellite position
                    if (el > 0)
                    {
                        float point_x = plot_size / 2;
                        float point_y = plot_size / 2;

                        point_x += az_el_to_plot_x(plot_size, radius, az, el);
                        point_y -= az_el_to_plot_y(plot_size, radius, az, el);

                        uint8_t color[3];
                        hsv_to_rgb(fmod(scid, 10) / 10.0, 1, 1, color);
                        img.draw_circle(point_x, point_y, 2, color, true);
                    }

                    if (ephem_time < 1741609584)
                        all_ephem_points.push_back({ephem_time, scid, az, el});
                }
            }
            else if (frm[i * 12] == 0x65)
            {
                if (fcs(&frm[i * 12], 24) == 0)
                {
                    int f = frm[i * 12 + 5];
                    logger->info("Synchronization, Freq %f Mhz", calcFreq(f));
                }
            }
            else if (frm[i * 12] == 0x1C)
            {
                if (fcs(&frm[i * 12], 12) == 0)
                {
                    int pos = frm[i * 12 + 1] & 0xF;

                    std::reverse(&frm[i * 12 + 2], &frm[i * 12 + 10]);
                    shift_array_left(&frm[i * 12 + 2], 8, 4, &frm[i * 12 + 2]);
                    uint16_t values[5];
                    repackBytesTo12bits(&frm[i * 12 + 2], 8, values);

                    std::string frequencies = "";
                    for (int i = 0; i < 5; i++)
                        if (values[i] != 0)
                            frequencies += std::to_string(calcFreq(values[i], false)) + " Mhz, ";
                    logger->info("Channels %d : %s", pos, frequencies.c_str());
                }
            }
        }
    }

    img.save_jpeg("orbcomm_plot.jpg");

    if (true)
    {
        time_t min_time = std::numeric_limits<time_t>::max();
        time_t max_time = 0;

        for (auto &ephem : all_ephem_points)
        {
            if (ephem.time < min_time)
                min_time = ephem.time;
            if (ephem.time > max_time)
                max_time = ephem.time;
        }

        logger->critical("Min Time " + timestamp_to_string(min_time));
        logger->critical("Max Time " + timestamp_to_string(max_time));

        sleep(5);

        int img_cnt = 1;

        image::Image<uint8_t> img(plot_size, plot_size + 50, 3);
        img.init_font("resources/fonts/font.ttf");
        for (time_t current_time = min_time; current_time < max_time; current_time += 10)
        {
            int plot_size = 512;

            // {
            // All black bg
            img.fill(0);

            // Draw the "target-like" plot with elevation rings
            float radius = 0.45;
            float radius1 = plot_size * radius * (3.0 / 9.0);
            float radius2 = plot_size * radius * (6.0 / 9.0);
            float radius3 = plot_size * radius * (9.0 / 9.0);

            uint8_t color_green[] = {0, 255, 0};
            uint8_t color_red[] = {255, 0, 0};
            uint8_t color_orange[] = {255, 165, 0};
            uint8_t color_cyan[] = {0, 237, 255};

            img.draw_circle(plot_size / 2, plot_size / 2,
                            radius1, color_green, false);
            img.draw_circle(plot_size / 2, plot_size / 2,
                            radius2, color_green, false);
            img.draw_circle(plot_size / 2, plot_size / 2,
                            radius3, color_green, false);

            img.draw_line(plot_size / 2, 0,
                          plot_size / 2, plot_size - 1,
                          color_green);
            img.draw_line(0, plot_size / 2,
                          plot_size - 1, plot_size / 2,
                          color_green);
            // }

            for (auto &ephem : all_ephem_points)
            {
                if (ephem.time < current_time && ephem.time > current_time - 240)
                {
                    // Draw the current satellite position
                    if (ephem.el > 0)
                    {
                        float point_x = plot_size / 2;
                        float point_y = plot_size / 2;

                        point_x += az_el_to_plot_x(plot_size, radius, ephem.az, ephem.el);
                        point_y -= az_el_to_plot_y(plot_size, radius, ephem.az, ephem.el);

                        uint8_t color[3];
                        hsv_to_rgb(fmod(ephem.scid, 10) / 10.0, 1, 1, color);
                        img.draw_circle(point_x, point_y, 2, color, true);
                    }
                }
            }

            uint8_t text_color[] = {255, 255, 255, 255};
            img.draw_text(130, plot_size + 10, text_color, 30, timestamp_to_string(current_time));

            img.save_jpeg("/data_ssd/orbcomm/orbcomm_movie/" + std::to_string(img_cnt++) + ".jpg");
            logger->trace(img_cnt);
        }
    }
}