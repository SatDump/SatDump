#include "att_ephem_reader.h"
#include "common/ccsds/ccsds_time.h"
#include "common/geodetic/ecef_to_eci.h"

namespace jpss
{
    namespace att_ephem
    {
        AttEphemReader::AttEphemReader()
        {
        }

        AttEphemReader::~AttEphemReader()
        {
        }

        inline double get_timestamp(uint8_t *dat)
        {
            uint16_t ephem_valid_time_days = dat[0] << 8 | dat[1];
            uint32_t ephem_valid_time_millis_of_days = dat[2] << 24 | dat[3] << 16 | dat[4] << 8 | dat[5];
            uint16_t ephem_valid_time_micros_of_millis = dat[6] << 8 | dat[7];
            return -378694800.0 + ephem_valid_time_days * 86400 + 3600 + double(ephem_valid_time_millis_of_days) / 1e3 + double(ephem_valid_time_micros_of_millis) / 1e6;
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

        void AttEphemReader::work(ccsds::CCSDSPacket &packet)
        {
            // Filter out bad packets
            if (packet.payload.size() != 65)
                return;

            uint8_t *dat = &packet.payload[8];

            // uint8_t scid = dat[0];

            double ephem_timestamp = get_timestamp(&dat[1]);
            double ephem_x = get_float(&dat[9]);
            double ephem_y = get_float(&dat[13]);
            double ephem_z = get_float(&dat[17]);
            double ephem_vx = get_float(&dat[21]);
            double ephem_vy = get_float(&dat[25]);
            double ephem_vz = get_float(&dat[29]);

            // double atti_timestamp = get_timestamp(&dat[33]);
            // float atti_q1 = get_float(&dat[41]);
            // float atti_q2 = get_float(&dat[45]);
            // float atti_q3 = get_float(&dat[49]);
            // float atti_q4 = get_float(&dat[53]);

            if (fabs(ephem_x) > 8000000 || fabs(ephem_y) > 8000000 || fabs(ephem_z) > 8000000)
                return;
            if (fabs(ephem_vx) > 8000000 || fabs(ephem_vy) > 8000000 || fabs(ephem_vz) > 8000000)
                return;

            // printf("%f - %f %f %f - %f %f %f\n",
            //        ephem_timestamp,
            //        ephem_x, ephem_y, ephem_z,
            //        ephem_vx, ephem_vy, ephem_vz);

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

        nlohmann::json AttEphemReader::getEphem()
        {
            return ephems;
        }
    }
}
