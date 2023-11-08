#include "gbad_reader.h"
#include "common/ccsds/ccsds_time.h"

#include "common/utils.h"

namespace aqua
{
    namespace gbad
    {
        GBADReader::GBADReader()
        {
        }

        GBADReader::~GBADReader()
        {
        }

        void swap(void *t, const void *s, size_t siz)
        {
            for (size_t k = 0; k < siz; k++)
                *((char *)t + k) = *((char *)s + siz - 1 - k);
        }

        int FormatConvert_schar(const char *ptr)
        {
            return (int)*(signed char *)(ptr);
        }

        unsigned short FormatConvert_ushort2(const char *ptr)
        {
            unsigned short t = 0;
            swap(&t, ptr, 2);
            return t;
        }

        long FormatConvert_long4(const char *ptr)
        {
            long t;
            int(*ptr) < 0 ? t = -1 : t = 0;
            swap(&t, ptr, 4);
            return t;
        }

        double MiL1750_EPFP(char *ptr)
        {
            long m1 = FormatConvert_long4(ptr); // first part of mantissa
            m1 /= 256;                          // remove exponent

            int exp = FormatConvert_schar(ptr + 3); // exponent

            unsigned short m2 = FormatConvert_ushort2(ptr + 4); // last  part of mantissa

            bool minusFlag = m1 < 0;
            if (minusFlag) // number will be negative
            {
                m1 = ~m1; // twos' complement mantissa
                m2 = ~m2 + 1;
                if (m2 == 0)
                    m1 += 0x00000001; // (carry 1 to m1)

                if (m1 < 0)
                    m1 = 1; // Must be 0x800000 = -1
            }               // (sign re-reversed later)

            const double c_2pM39 = pow(2.0, -39); // LSB of last  part of mantissa,
            const double c_2pM23 = pow(2.0, -23); // LSB of first part of mantissa
                                                  // after /256 to remove exponent.

            double v1 = m1 * c_2pM23; // value from first part mantissa
            double v2 = m2 * c_2pM39; // value from last  part mantissa

            // Scale to characteristic and
            v1 = (v1 + v2) * pow(2.0, exp); // set sign
            if (minusFlag)
                v1 = -v1;

            return v1;
        }

        void GBADReader::work(ccsds::CCSDSPacket &packet)
        {
            // Filter out bad packets
            if (packet.payload.size() != 120)
                return;

            uint32_t time_coarse = FormatConvert_long4((char *)&packet.payload[2]); // seconds
            uint16_t time_fine = FormatConvert_ushort2((char *)&packet.payload[6]);
            double ephem_timestamp = -378694800.0 + time_coarse + 3600 + time_fine * 15.2e-6;

            double ephem_x = MiL1750_EPFP((char *)&packet.payload[8 + 6 * 0]);
            double ephem_y = MiL1750_EPFP((char *)&packet.payload[8 + 6 * 1]);
            double ephem_z = MiL1750_EPFP((char *)&packet.payload[8 + 6 * 2]);
            double ephem_vx = MiL1750_EPFP((char *)&packet.payload[8 + 6 * 3]);
            double ephem_vy = MiL1750_EPFP((char *)&packet.payload[8 + 6 * 4]);
            double ephem_vz = MiL1750_EPFP((char *)&packet.payload[8 + 6 * 5]);

            if (fabs(ephem_vx / 1e3) > 10)
                return;
            if (fabs(ephem_vy / 1e3) > 10)
                return;
            if (fabs(ephem_vz / 1e3) > 10)
                return;

            if (fabs(ephem_x / 1e3) > 1e4)
                return;
            if (fabs(ephem_y / 1e3) > 1e4)
                return;
            if (fabs(ephem_z / 1e3) > 1e4)
                return;

            // Convert to km from meters
            ephems[ephems_n]["timestamp"] = ephem_timestamp;
            ephems[ephems_n]["x"] = ephem_x / 1e3;
            ephems[ephems_n]["y"] = ephem_y / 1e3;
            ephems[ephems_n]["z"] = ephem_z / 1e3;
            ephems[ephems_n]["vx"] = ephem_vx / 1e3;
            ephems[ephems_n]["vy"] = ephem_vy / 1e3;
            ephems[ephems_n]["vz"] = ephem_vz / 1e3;
            ephems_n++;

            //  printf("GBAD!!! %s - %f %f %f - %f %f %f\n", timestamp_to_string(ephem_timestamp).c_str(),
            //         ephem_x / 1e3, ephem_y / 1e3, ephem_z / 1e3,
            //         ephem_vx / 1e3, ephem_vy / 1e3, ephem_vz / 1e3);
        }

        nlohmann::json GBADReader::getEphem()
        {
            return ephems;
        }
    }
}
