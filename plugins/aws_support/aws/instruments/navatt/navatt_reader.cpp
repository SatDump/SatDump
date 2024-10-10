#include "navatt_reader.h"
#include "common/ccsds/ccsds_time.h"
#include "common/geodetic/ecef_to_eci.h"

#include "logger.h"
#include "common/utils.h"
#include "nlohmann/json.hpp"
#include <cstdint>

namespace aws
{
    namespace navatt
    {
        NavAttReader::NavAttReader()
        {
		lines = 0;
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
	    std::memcpy(array, packet.payload.data(), 116);

            uint8_t tmVersionNumber = array[0] >> 4;
	    uint8_t spacecraftTimeRefStat = array[0] & 0b1111;
	    uint8_t serviceType = array[1];
	    uint8_t serviceSubType = array[2];
	    uint16_t messageTypeCounter = array[3] << 8 | array[4];
	    uint16_t destinationID = array[5] << 8 | array[6];
	    uint8_t preamble = array[7];
	    uint32_t cucTimeSecs = array[8] << 24 | array[9] << 16 | array[10] << 8 | array[11];
	    uint32_t cucTimeFrac = array[12] << 16 | array[13] << 8 | array[14];

	    // Data field
	    uint16_t sid = array[15] << 8 | array[16];

	    double navTimestamp = get_double(&array[17]);

	    uint32_t orbitNumber = array[25] << 16 | array[26] << 8 | array[27];
	    uint32_t orbitFraction = array[28] << 24 | array[29] << 16 | array[30] << 8 | array[31];
	    uint8_t posVelQuality = array[32];

            double ephem_timestamp = get_double(&dat[0]) + 3657 * 24 * 3600;
            double ephem_x = get_float(&dat[33]);
            double ephem_y = get_float(&dat[37]);
            double ephem_z = get_float(&dat[41]);
            double ephem_vx = get_float(&dat[45]);
            double ephem_vy = get_float(&dat[49]);
            double ephem_vz = get_float(&dat[53]);

	    uint8_t attitudeQual = array[65];

	    double quaternion1 = get_double(&array[66]);
	    double quaternion2 = get_double(&array[74]);
	    double quaternion3 = get_double(&array[82]);
	    double quaternion4 = get_double(&array[90]);

	    float xRate = get_float(&array[98]);
	    float yRate = get_float(&array[102]);
	    float zRate = get_float(&array[106]);

	    uint8_t spacecraftMode = array[110];
	    uint8_t manoeuvreFlag = array[111];
	    uint8_t payloadMode = array[112];
	    uint16_t scid = array[113] << 8 | array[114];

	    double epochTimestamp = cucTimeSecs + cucTimeFrac / 16777215.0 + 3657 * 24 * 3600;

	    telemetry["TM Version Number"].push_back(tmVersionNumber);

	    if (spacecraftTimeRefStat == 0b0000)
	            telemetry["Spacecraft Time Reference Status"].push_back("Undefined");
	    else if (spacecraftTimeRefStat == 0b0001)
	            telemetry["Spacecraft Time Reference Status"].push_back("Not synchronized");
	    else if (spacecraftTimeRefStat == 0b0010)
	            telemetry["Spacecraft Time Reference Status"].push_back("Coarse");
	    else if (spacecraftTimeRefStat == 0b0011)
	            telemetry["Spacecraft Time Reference Status"].push_back("Coarse From Fine");
	    else if (spacecraftTimeRefStat == 0b0100)
	            telemetry["Spacecraft Time Reference Status"].push_back("Fine");
	    if (serviceType == 0b0011)
	            telemetry["Service Type"].push_back("NAVATT");
	    else if (serviceType == 0b10000000)
	            telemetry["Service Type"].push_back("SCIENCE");

	    telemetry["Service Sub-type"].push_back(serviceSubType);
	    telemetry["Message Type Counter"].push_back(messageTypeCounter);

	    if (destinationID == 0b0)
	            telemetry["Destination ID"].push_back("Ground");
	    else if (destinationID == 0b1)
	            telemetry["Destination ID"].push_back("On-board");

	    telemetry["Preamble"].push_back(preamble);
	    telemetry["CUC Time Seconds"].push_back(cucTimeSecs);
	    telemetry["CUC Time Fraction"].push_back(cucTimeFrac);
	    telemetry["Epoch Timestamp"].push_back(epochTimestamp);

	    // Data field
	    telemetry["SID"].push_back(sid);
	    telemetry["Navigation timestamp"].push_back(navTimestamp);
	    telemetry["Orbit Number"].push_back(orbitNumber);
	    telemetry["Orbit Fraction"].push_back(orbitFraction);

	    if (posVelQuality == 0b00)
	            telemetry["Position Velocity Quality"].push_back("None");
	    else if (posVelQuality == 0b01)
	            telemetry["Position Velocity Quality"].push_back("TLE");
	    else if (posVelQuality == 0b10)
	            telemetry["Position Velocity Quality"].push_back("Propagation");
	    else if (posVelQuality == 0b11)
	            telemetry["Position Velocity Quality"].push_back("GNSS");

	    telemetry["X position"].push_back(ephem_x);
	    telemetry["Y position"].push_back(ephem_y);
	    telemetry["Z position"].push_back(ephem_z);
	    telemetry["X Velocity"].push_back(ephem_vx);
	    telemetry["Y Velocity"].push_back(ephem_vy);
	    telemetry["Z Velocity"].push_back(ephem_vz);
	    telemetry["Attitude timestamp"].push_back(ephem_timestamp);

	    if (attitudeQual == 0b0000)
	            telemetry["Attitude Quality"].push_back("None");
	    else if (attitudeQual == 0b0001)
	            telemetry["Attitude Quality"].push_back("Torque only");
	    else if (attitudeQual == 0b0010)
	            telemetry["Attitude Quality"].push_back("GYR only");
	    else if (attitudeQual == 0b0011)
	            telemetry["Attitude Quality"].push_back("ST1 only");
	    else if (attitudeQual == 0b0100)
	            telemetry["Attitude Quality"].push_back("ST2 only");
	    else if (attitudeQual == 0b0101)
	            telemetry["Attitude Quality"].push_back("ST1 & ST2");
	    else if (attitudeQual == 0b0111)
	            telemetry["Attitude Quality"].push_back("ST1 & GYR only");
	    else if (attitudeQual == 0b1000)
	            telemetry["Attitude Quality"].push_back("ST1 & GYR only");
	    else if (attitudeQual == 0b1001)
	            telemetry["Attitude Quality"].push_back("ST1 & ST2 & GYR");

	    telemetry["Quaternion 1"].push_back(quaternion1);
	    telemetry["Quaternion 2"].push_back(quaternion2);
	    telemetry["Quaternion 3"].push_back(quaternion3);
	    telemetry["Quaternion 4"].push_back(quaternion4);
	    telemetry["X Rate"].push_back(xRate);
	    telemetry["Y Rate"].push_back(yRate);
	    telemetry["Z Rate"].push_back(zRate);

	    if (spacecraftMode == 0b0111)
	            telemetry["Spacecraft Mode"].push_back("Orbit Maintenance Mode");
	    else if (spacecraftMode == 0b1000)
	            telemetry["Spacecraft Mode"].push_back("Payload Operations Mode");
	    if (manoeuvreFlag == 0b00)
	            telemetry["Manoeuvre Flag"].push_back("None");
	    else if (manoeuvreFlag == 0b01)
	            telemetry["Manoeuvre Flag"].push_back("EP Firing");
	    else if (manoeuvreFlag == 0b10)
	            telemetry["Manoeuvre Flag"].push_back("Momentum Management");
	    else if (manoeuvreFlag == 0b11)
	            telemetry["Manoeuvre Flag"].push_back("EP Firing + Momentum Management");
	    if (payloadMode == 0b0000)
	            telemetry["Payload Mode"].push_back("Off");
	    else if (payloadMode == 0b0001)
	            telemetry["Payload Mode"].push_back("Bootloader");
	    else if (payloadMode == 0b0010)
	            telemetry["Payload Mode"].push_back("Startup");
	    else if (payloadMode == 0b0011)
	            telemetry["Payload Mode"].push_back("Operation");
	    else if (payloadMode == 0b0100)
	            telemetry["Payload Mode"].push_back("Safe");
	    else if (payloadMode == 0b0101)
	            telemetry["Payload Mode"].push_back("Test");

	    telemetry["SCID"].push_back(scid);

	    lines++;

            //logger->info("NAVATT! %s", timestamp_to_string(ephem_timestamp).c_str());

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

            //printf("%f - %f %f %f - %f %f %f\n",
            //       ephem_timestamp,
            //       ephem_x, ephem_y, ephem_z,
            //       ephem_vx, ephem_vy, ephem_vz);

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

	nlohmann::ordered_json NavAttReader::dump_telemetry()
	{
		return telemetry;
	}

        nlohmann::json NavAttReader::getEphem()
        {
            return ephems;
        }
    }
}
