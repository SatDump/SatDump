#include "navatt_reader.h"
#include "common/ccsds/ccsds_time.h"
#include "common/geodetic/ecef_to_eci.h"

#include "logger.h"
#include "common/utils.h"

namespace aws
{
    namespace navatt
    {
        NavAttReader::NavAttReader()
        {
        }

        NavAttReader::~NavAttReader()
        {
        }

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

            double ephem_timestamp = get_double(&dat[0]) + 3657 * 24 * 3600;
            double ephem_x = get_float(&dat[33]);
            double ephem_y = get_float(&dat[37]);
            double ephem_z = get_float(&dat[41]);
            double ephem_vx = get_float(&dat[45]);
            double ephem_vy = get_float(&dat[49]);
            double ephem_vz = get_float(&dat[53]);

            logger->info("NAVATT! %s", timestamp_to_string(ephem_timestamp).c_str());

#if 0
            // double atti_timestamp = get_timestamp(&dat[33]);
            // float atti_q1 = get_float(&dat[41]);
            // float atti_q2 = get_float(&dat[45]);
            // float atti_q3 = get_float(&dat[49]);
            // float atti_q4 = get_float(&dat[53]);
#endif

            if (fabs(ephem_x) > 8000000 || fabs(ephem_y) > 8000000 || fabs(ephem_z) > 8000000)
                return;
            if (fabs(ephem_vx) > 8000000 || fabs(ephem_vy) > 8000000 || fabs(ephem_vz) > 8000000)
                return;

            printf("%f - %f %f %f - %f %f %f\n",
                   ephem_timestamp,
                   ephem_x, ephem_y, ephem_z,
                   ephem_vx, ephem_vy, ephem_vz);

            ecef_epehem_to_eci(ephem_timestamp, ephem_x, ephem_y, ephem_z, ephem_vx, ephem_vy, ephem_vz);

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

        nlohmann::json NavAttReader::getEphem()
        {
            return ephems;
        }
    }
}
