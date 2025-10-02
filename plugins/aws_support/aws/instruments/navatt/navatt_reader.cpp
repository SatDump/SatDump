#include "navatt_reader.h"
#include "common/ccsds/ccsds_time.h"
#include "common/geodetic/ecef_to_eci.h"

#include "common/tracking/tle.h"
#include "common/tracking/tracking.h"
#include "common/utils.h"
#include "init.h"

extern "C"
{
#include "libs/supernovas/novas.h"

#include "libs/calceph/calceph.h"
#include "libs/supernovas/novas-calceph.h"
}

#include "logger.h"
#include "utils/time.h"

#define LEAP_SECONDS 37

// TODOREWORKDB

namespace aws
{
    namespace navatt
    {
        NavAttReader::NavAttReader() {}

        NavAttReader::~NavAttReader() {}

        inline float get_float(uint8_t *dat)
        {
            float v = 0;
            ((uint8_t *)&v)[3] = dat[0];
            ((uint8_t *)&v)[2] = dat[1];
            ((uint8_t *)&v)[1] = dat[2];
            ((uint8_t *)&v)[0] = dat[3];
            return v;
        }

        inline double get_double(uint8_t *dat)
        {
            double v = 0;
            ((uint8_t *)&v)[7] = dat[0];
            ((uint8_t *)&v)[6] = dat[1];
            ((uint8_t *)&v)[5] = dat[2];
            ((uint8_t *)&v)[4] = dat[3];
            ((uint8_t *)&v)[3] = dat[4];
            ((uint8_t *)&v)[2] = dat[5];
            ((uint8_t *)&v)[1] = dat[6];
            ((uint8_t *)&v)[0] = dat[7];
            return v;
        }

        void NavAttReader::work(ccsds::CCSDSPacket &packet)
        {
            // Filter out bad packets
            if (packet.payload.size() != 117)
                return;

            uint8_t *dat = &packet.payload[21 - 6 + 2];

            double ephem_timestamp = get_double(&dat[0]) + 3657 * 24 * 3600 - 16;
            double ephem_x = get_float(&dat[16]);
            double ephem_y = get_float(&dat[20]);
            double ephem_z = get_float(&dat[24]);
            double ephem_vx = get_float(&dat[28]);
            double ephem_vy = get_float(&dat[32]);
            double ephem_vz = get_float(&dat[36]);

            // logger->info("NAVATT! %s", timestamp_to_string(ephem_timestamp).c_str());

#if 0
            // double atti_timestamp = get_timestamp(&dat[33]);
            // float atti_q1 = get_float(&dat[41]);
            // float atti_q2 = get_float(&dat[45]);
            // float atti_q3 = get_float(&dat[49]);
            // float atti_q4 = get_float(&dat[53]);
#endif

            // if (fabs(ephem_x) > 8000000 || fabs(ephem_y) > 8000000 || fabs(ephem_z) > 8000000)
            //     return;
            // if (fabs(ephem_vx) > 8000000 || fabs(ephem_vy) > 8000000 || fabs(ephem_vz) > 8000000)
            //     return;

            satdump::SatelliteTracker track(satdump::db_tle->get_from_norad(60543).value());
            auto p = track.get_sat_position_at_raw(ephem_timestamp);

            // printf("%f - %f %f %f - %f %f %f\n", ephem_timestamp, ephem_x, ephem_y, ephem_z, ephem_vx, ephem_vy, ephem_vz);

            {
                if (ephems_n == 0)
                {
                    const char *arrr[] = {"/home/alan/Downloads/de440s.bsp"};
                    t_calcephbin *de440 = calceph_open_array(1, arrr); //// calceph_open("/home/alan/Downloads/de440s.bsp");
                    if (!de440)
                        fprintf(stderr, "ERROR! could not open ephemeris data\n");
                    novas_use_calceph(de440);
                }

                struct timespec unix_time;
                double x = ephem_timestamp;
                unix_time.tv_sec = (long)x;
                unix_time.tv_nsec = (x - unix_time.tv_sec) * 1000000000L;

                novas_timespec obs_time; // astrometric time of observation

                auto iers = satdump::db_iers->getBestIERSInfo(ephem_timestamp);

                if (novas_set_unix_time(unix_time.tv_sec, unix_time.tv_nsec, LEAP_SECONDS, iers.ut1_utc, &obs_time) != 0)
                    fprintf(stderr, "ERROR! failed to set time of observation.\n");

                // char timestamp1[40];
                // novas_iso_timestamp(&obs_time, timestamp1, sizeof(timestamp1));
                // logger->critical(timestamp1);

                observer obs;
                if (make_observer_at_geocenter(&obs))
                    fprintf(stderr, "ERROR! defining Earth-based observer location.\n");

                novas_frame obs_frame;
                if (novas_make_frame(NOVAS_FULL_ACCURACY, &obs, &obs_time, iers.polar_dx, iers.polar_dy, &obs_frame) != 0)
                    fprintf(stderr, "ERROR! failed to define observing frame.\n");

                {
                    double s = (1. / NOVAS_AU);
                    // printf("AU %f\n", s);
                    double s2 = NOVAS_AU;
                    double pos[3] = {ephem_x * s, ephem_y * s, ephem_z * s};
                    double pos2[3];
                    novas_transform testt;
                    novas_make_transform(&obs_frame, NOVAS_J2000, NOVAS_ITRS, &testt);
                    novas_transform_vector(pos, &testt, pos2);
                    ephem_x = pos2[0] * s2;
                    ephem_y = pos2[1] * s2;
                    ephem_z = pos2[2] * s2;
                }

                {
                    double s = (1. / NOVAS_AU);
                    // printf("AU %f\n", s);
                    double s2 = NOVAS_AU;
                    double pos[3] = {ephem_vx * s, ephem_vy * s, ephem_vz * s};
                    double pos2[3];
                    novas_transform testt;
                    novas_make_transform(&obs_frame, NOVAS_J2000, NOVAS_ITRS, &testt);
                    novas_transform_vector(pos, &testt, pos2);
                    ephem_vx = pos2[0] * s2;
                    ephem_vy = pos2[1] * s2;
                    ephem_vz = pos2[2] * s2;
                }
            }

            ecef_epehem_to_eci(ephem_timestamp, ephem_x, ephem_y, ephem_z, ephem_vx, ephem_vy, ephem_vz);

            printf("%f  - CALC %f %f %f - %f %f %f ///////////// RAW %f %f %f - %f %f %f\n", ephem_timestamp, //
                   p.position[0], p.position[1], p.position[2], p.velocity[0], p.velocity[1], p.velocity[2],  //
                   ephem_x, ephem_y, ephem_z, ephem_vx, ephem_vy, ephem_vz);

            // Convert to km from meters
            ephems[ephems_n]["timestamp"] = ephem_timestamp;
            ephems[ephems_n]["x"] = ephem_x;
            ephems[ephems_n]["y"] = ephem_y;
            ephems[ephems_n]["z"] = ephem_z;
            ephems[ephems_n]["vx"] = ephem_vx;
            ephems[ephems_n]["vy"] = ephem_vy;
            ephems[ephems_n]["vz"] = ephem_vz;
            ephems_n++;
        }

        nlohmann::json NavAttReader::getEphem() { return ephems; }
    } // namespace navatt
} // namespace aws
