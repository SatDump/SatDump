#include "dump_svissr_telemetry.h"

namespace fengyun_svissr
{
    std::string SVISSRTelemetryDumper::dump_telemetry_to_JSON(OrbitAndAttitudeData OAT_block, AttitudePredictionSubBlock Attitude_subblock, OrbitPredictionSubBlock Orbital_subblock,
                                                              std::string active_sensor, uint8_t active_detectors)
    {
        // There is DEFINITELY a better way to do this
        // but i don't know it so lord have mercy

        std::string output = "{\n";
        output += "    \"Active VISSR sensor\": \"" + active_sensor + "\",\n";

        // Handling for active detectors
        output += "    \"Active VIS1 detector\": ";
        output += (active_detectors & 0b10000) == 0b10000 ? "\"Primary\"" : "\"Backup\"";
        output += ",\n";

        output += "    \"Active VIS2 detector\": ";
        output += (active_detectors & 0b100000) == 0b100000 ? "\"Primary\"" : "\"Backup\"";
        output += ",\n";

        output += "    \"Active VIS3 detector\": ";
        output += (active_detectors & 0b1000000) == 0b1000000 ? "\"Primary\"" : "\"Backup\"";
        output += ",\n";

        output += "    \"Active VIS4 detector\": ";
        output += (active_detectors & 0b10000000) == 0b10000000 ? "\"Primary\"" : "\"Backup\"";
        output += ",\n";

        output += "    \"Active IR1 detector\": ";
        output += (active_detectors & 0b1) == 0b1 ? "\"Primary\"" : "\"Backup\"";
        output += ",\n";

        output += "    \"Active IR2 detector\": ";
        output += (active_detectors & 0b10) == 0b10 ? "\"Primary\"" : "\"Backup\"";
        output += ",\n";

        output += "    \"Active IR3 detector\": ";
        output += (active_detectors & 0b100) == 0b100 ? "\"Primary\"" : "\"Backup\"";
        output += ",\n";

        output += "    \"Active IR4 detector\": ";
        output += (active_detectors & 0b1000) == 0b1000 ? "\"Primary\"" : "\"Backup\"";
        output += ",\n";

        output += "    \"Image start time [Unix]\": " + std::to_string(((OAT_block.IMAGE_START_TIME * 1e-8)- 40587) * 86400) + ",\n";
        output += "    \"VIS channel stepping along line [rad]\": " + std::to_string(OAT_block.VIS_STEPPING_LINE * 1e-8) + ",\n";
        output += "    \"IR channel stepping along line [rad]\": " + std::to_string(OAT_block.IR_STEPPING_LINE * 1e-8) + ",\n";
        output += "    \"VIS channel stepping along pixel [rad]\": " + std::to_string(OAT_block.VIS_STEPPING_PIXEL * 1e-10) + ",\n";
        output += "    \"IR channel stepping along pixel [rad]\": " + std::to_string(OAT_block.IR_STEPPING_PIXEL * 1e-10) + ",\n";
        output += "    \"VIS channel center line number\": " + std::to_string(OAT_block.VIS_CENTER_LINE * 1e-4) + ",\n";
        output += "    \"IR channel center line number\": " + std::to_string(OAT_block.IR1_CENTER_LINE * 1e-4) + ",\n";
        output += "    \"IR2 channel center line number\": " + std::to_string(OAT_block.IR2_channel_center_line_number_of_frame * 1e-4) + ",\n";
        output += "    \"IR3 channel center line number\": " + std::to_string(OAT_block.IR3_channel_center_line_number_of_frame * 1e-4) + ",\n";
        output += "    \"VIS channel center pixel number\": " + std::to_string(OAT_block.VIS_CENTER_PIXEL * 1e-4) + ",\n";
        output += "    \"IR1 channel center pixel number\": " + std::to_string(OAT_block.IR1_CENTER_PIXEL * 1e-4) + ",\n";
        output += "    \"Number of VIS sensors\": " + std::to_string(OAT_block.VIS_SENSOR_NUMBER * 1e-0) + ",\n";
        output += "    \"Number of IR sensors\": " + std::to_string(OAT_block.IR_SENSOR_NUMBER * 1e-0) + ",\n";
        output += "    \"Total number of VIS channel lines\": " + std::to_string(OAT_block.VIS_TOTAL_LINE_NUMBER * 1e-0) + ",\n";
        output += "    \"Total number of IR channel lines\": " + std::to_string(OAT_block.IR_TOTAL_LINE_NUMBER * 1e-0) + ",\n";
        output += "    \"Pixels per VIS line\": " + std::to_string(OAT_block.VIS_LINE_PX * 1e-0) + ",\n";
        output += "    \"Pixels per IR line\": " + std::to_string(OAT_block.IR_LINE_PX * 1e-0) + ",\n";
        output += "    \"VISSR misalignment angle around the x axis [rad]\": " + std::to_string(OAT_block.VISSR_MISALIGNMENT_ANGLE_X_AXIS * 1e-10) + ",\n";
        output += "    \"VISSR misalignment angle around the y axis [rad]\": " + std::to_string(OAT_block.VISSR_MISALIGNMENT_ANGLE_Y_AXIS * 1e-10) + ",\n";
        output += "    \"VISSR misalignment angle around the z axis [rad]\": " + std::to_string(OAT_block.VISSR_MISALIGNMENT_ANGLE_Z_AXIS * 1e-10) + ",\n";
        output += "    \"VISSR misalignment matrix row 1 column 1 (Constant)\": " + std::to_string(OAT_block.VMM_R1_C1 * 1e-7) + ",\n";
        output += "    \"VISSR misalignment matrix row 2 column 1 (Constant)\": " + std::to_string(OAT_block.VMM_R2_C1 * 1e-10) + ",\n";
        output += "    \"VISSR misalignment matrix row 3 column 1 (Constant)\": " + std::to_string(OAT_block.VMM_R3_C1 * 1e-10) + ",\n";
        output += "    \"VISSR misalignment matrix row 1 column 2 (Constant)\": " + std::to_string(OAT_block.VMM_R1_C2 * 1e-10) + ",\n";
        output += "    \"VISSR misalignment matrix row 2 column 2 (Constant)\": " + std::to_string(OAT_block.VMM_R2_C2 * 1e-7) + ",\n";
        output += "    \"VISSR misalignment matrix row 3 column 2 (Constant)\": " + std::to_string(OAT_block.VMM_R3_C2 * 1e-10) + ",\n";
        output += "    \"VISSR misalignment matrix row 1 column 3 (Constant)\": " + std::to_string(OAT_block.VMM_R1_C3 * 1e-10) + ",\n";
        output += "    \"VISSR misalignment matrix row 2 column 3 (Constant)\": " + std::to_string(OAT_block.VMM_R2_C3 * 1e-10) + ",\n";
        output += "    \"VISSR misalignment matrix row 3 column 3 (Constant)\": " + std::to_string(OAT_block.VMM_R3_C3 * 1e-7) + ",\n";
        output += "    \"Ratio of circumference (Constant)\": " + std::to_string(OAT_block.RATIO_OF_CIRCUMFERENCE * 1e-7) + ",\n";
        output += "    \"Earth equatorial radius (Constant) [m]\": " + std::to_string(OAT_block.EARTH_RADIUS * 1e-1) + ",\n";
        output += "    \"Earth oblateness (Constant)\": " + std::to_string(OAT_block.EARTH_OBLATENESS * 1e-10) + ",\n";
        output += "    \"Earth eccentricity (Constant)\": " + std::to_string(OAT_block.EARTH_ECCENTRICITY * 1e-9) + ",\n";
        output += "    // Orbital parameters start here)\n";
        output += "    \"Angle between VISSR and view of sun sensor at start of data (Constant?) [rad]\": " + std::to_string(OAT_block.VISSR_SUN_SENSOR_ANGLE * 1e-8) + ",\n";
        output += "    \"Epoch of orbital parameters in mean J2K [MJD]\": " + std::to_string(OAT_block.ORBITAL_PARAM_EPOCH * 1e-8) + ",\n";
        output += "    \"Semi major axis in mean J2K [km]\": " + std::to_string(OAT_block.SEMI_MAJOR_AXIS * 1e-8) + ",\n";
        output += "    \"Eccentricity in mean J2K\": " + std::to_string(OAT_block.ECCENTRICITY * 1e-10) + ",\n";
        output += "    \"Inclination in mean J2K [deg]\": " + std::to_string(OAT_block.INCLINATION * 1e-8) + ",\n";
        output += "    \"Longitude of ascending node in mean J2K [deg]\": " + std::to_string(OAT_block.LONGITUDE_ASCENDING_NODE * 1e-8) + ",\n";
        output += "    \"Argument of perigee in mean J2K [deg]\": " + std::to_string(OAT_block.PERIGEE * 1e-8) + ",\n";
        output += "    \"Mean anomaly in mean J2K [deg]\": " + std::to_string(OAT_block.MEAN_ANOMALY * 1e-8) + ",\n";
        output += "    \"Sub-satellite East Longitude in mean J2K [deg]\": " + std::to_string(OAT_block.SSP_LONGITUDE * 1e-6) + ",\n";
        output += "    \"Sub-satellite North latitude in mean J2K [deg]\": " + std::to_string(OAT_block.SSP_LATITUDE * 1e-6) + ",\n";
        output += "    // Attitude parameters start here)\n";
        output += "    \"Epoch of attitude parameters in mean J2K [MJD]\": " + std::to_string(OAT_block.ATTITUDE_PARAM_EPOCH * 1e-8) + ",\n";
        output += "    \"Angle between Z-axis and satellite spin axis projected on yz-plane alpha in mean J2K [rad]\": " + std::to_string(OAT_block.ALPHA_Z_AXIS_SAT_SPIN_ON_YZ_PLANE * 1e-8) + ",\n";
        output += "    \"Change rate of alpha in mean J2K\": " + std::to_string(OAT_block.ALPHA_CHANGE_RATE * 1e-15) + ",\n";
        output += "    \"Angle between Satellite Spin Axis and yz-plane delta in mean J2K [rad]\": " + std::to_string(OAT_block.DELTA_SAT_SPIN_YZ_PLANE * 1e-11) + ",\n";
        output += "    \"Change rate of delta in mean J2K\": " + std::to_string(OAT_block.DELTA_CHANGE_RATE * 1e-15) + ",\n";
        output += "    \"Estimated daily mean spin rate [rpm]\": " + std::to_string(OAT_block.MEAN_SPIN_RATE * 1e-8) + ",\n";
        output += "    // Attitude calculation info starts here)\n";
        output += "    \"Time of the first attitude prediction record [Unix]\": " + std::to_string(((OAT_block.FIRST_ATTITUDE_PREDICTION * 1e-8)- 40587) * 86400) + ",\n";
        output += "    \"Time of the latest attitude prediction record [Unix]\": " + std::to_string(((OAT_block.LATEST_ATTITUDE_PREDICTION * 1e-8)- 40587) * 86400) + ",\n";
        output += "    \"Interval time of attitude prediction data (Constant at 0.00347222) [MJD]\": " + std::to_string(OAT_block.ATTITUDE_PREDICTION_INTERVAL * 1e-8) + ",\n";
        output += "    \"Attitude prediction number\": " + std::to_string(OAT_block.ATTITUDE_PREDICTION_NUMBER) + ",\n";
        output += "    \"Time of first orbit prediction record [Unix]\": " + std::to_string(((OAT_block.FIRST_ORBITAL_PREDICTION * 1e-8)- 40587) * 86400) + ",\n";
        output += "    \"Time of last orbit prediction record [Unix]\": " + std::to_string(((OAT_block.LATEST_ORBITAL_PREDICTION * 1e-8)- 40587) * 86400) + ",\n";
        output += "    \"Interval of orbital prediction data (Constant at 0.00347222) [MJD]\": " + std::to_string(OAT_block.ORBITAL_PREDICTION_INTERVAL * 1e-8) + ",\n";
        output += "    \"Orbital prediction number [=8?]\": " + std::to_string(OAT_block.ORBITAL_PREDICTION_COUNT) + ",\n";
        output += "    // Attitude prediction data sub-block starts here\n";
        output += "    \"UTC time of attitude prediction processing [Unix]\": " + std::to_string(((Attitude_subblock.PREDICTION_TIME * 1e-8)- 40587) * 86400) + ",\n";
        output += "    \"UTC time of attitude prediction processing [BCD]\": " + std::to_string(Attitude_subblock.PREDICTION_TIME_BCD) + ",\n";
        output += "    \"Angle between z-axis and satellite spin axis projected on yz plane in mean of 1950.0 coordinates, mean J2K [rad]\": " +
                  std::to_string(Attitude_subblock.Z_AXIS_SAT_SPIN_ON_YZ_PLANE_ANGLE * 1e-8) + ",\n";
        output += "    \"Angle between satelltie spin axis and yz-plane, [rad]\": " + std::to_string(Attitude_subblock.SAT_SPIN_YZ_PLANE_ANGLE * 1e-11) + ",\n";
        output += "    \"Dihedral angle between the Sun and Earth measured clockwise viewing from North, [rad]\": " + std::to_string(Attitude_subblock.DIHEDRAL_ANGLE * 1e-8) + ",\n";
        output += "    \"Spin speed of satellite [rpm]\": " + std::to_string(Attitude_subblock.SPIN_RATE * 1e-8) + ",\n";
        output += "    \"Right ascension of satellite spin axis on the sat orbit plane coordinate system [rad]\": " + std::to_string(Attitude_subblock.RIGHT_ASCENSION_SPIN_AXIS * 1e-8) + ",\n";
        output += "    \"Declination of satellite spin axis on the sat orbit plane coordinate system [rad]\": " + std::to_string(Attitude_subblock.DECLINATION_SPIN_AXIS * 1e-8) + ",\n";
        output += "    // Orbit prediction sub-block starts here\n";
        output += "    \"UTC time of orbit prediction processing [Unix]\": " + std::to_string(((Orbital_subblock.PREDICTION_TIME_MJD * 1e-8)- 40587) * 86400) + ",\n";
        output += "    \"UTC time of orbit prediction processing [BCD]\": " + std::to_string(Orbital_subblock.PREDICTION_TIME_BCD) + ",\n";
        output += "    \"X component of the satellite position, mean J2K [m]\": " + std::to_string(Orbital_subblock.X_COMP * 1e-6) + ",\n";
        output += "    \"Y component of the satellite position, mean J2K [m]\": " + std::to_string(Orbital_subblock.Y_COMP * 1e-6) + ",\n";
        output += "    \"Z component of the satellite position, mean J2K [m]\": " + std::to_string(Orbital_subblock.Z_COMP * 1e-6) + ",\n";
        output += "    \"X component of the satellite position, mean J2K [m/s]\": " + std::to_string(Orbital_subblock.X_COMP_SPEED * 1e-8) + ",\n";
        output += "    \"Y component of the satellite position, mean J2K [m/s]\": " + std::to_string(Orbital_subblock.Y_COMP_SPEED * 1e-8) + ",\n";
        output += "    \"Z component of the satellite position, mean J2K [m/s]\": " + std::to_string(Orbital_subblock.Z_COMP_SPEED * 1e-8) + ",\n";
        output += "    \"X component of the satellite position, ECEF [m]\": " + std::to_string(Orbital_subblock.X_COMP_EARTH_FIXED * 1e-6) + ",\n";
        output += "    \"Y component of the satellite position, ECEF [m]\": " + std::to_string(Orbital_subblock.Y_COMP_EARTH_FIXED * 1e-6) + ",\n";
        output += "    \"Z component of the satellite position, ECEF [m]\": " + std::to_string(Orbital_subblock.Z_COMP_EARTH_FIXED * 1e-6) + ",\n";
        output += "    \"X component of the satellite position, ECEF [m/s]\": " + std::to_string(Orbital_subblock.X_COMP_EARTH_FIXED_SPEED * 1e-10) + ",\n";
        output += "    \"Y component of the satellite position, ECEF [m/s]\": " + std::to_string(Orbital_subblock.Y_COMP_EARTH_FIXED_SPEED * 1e-10) + ",\n";
        output += "    \"Z component of the satellite position, ECEF [m/s]\": " + std::to_string(Orbital_subblock.Z_COMP_EARTH_FIXED_SPEED * 1e-10) + ",\n";
        output += "    \"Greenwhich sidereal time in true of data coordinates, [deg]\": " + std::to_string(Orbital_subblock.GREENWHICH_SIDEREAL * 1e-8) + ",\n";
        output += "    \"Right ascension from the satellite to the sun in mean J2K [deg]\": " + std::to_string(Orbital_subblock.RATS_J2 * 1e-8) + ",\n";
        output += "    \"Declination from the satellite to the sun in J2K [deg]\": " + std::to_string(Orbital_subblock.DSTS_J2 * 1e-8) + ",\n";
        output += "    \"Right ascension from the satellite to the sun in ECEF [deg]\": " + std::to_string(Orbital_subblock.RATS_EF * 1e-8) + ",\n";
        output += "    \"Declination from the satellite to the sun in ECEF [deg]\": " + std::to_string(Orbital_subblock.DSTS_EF * 1e-8) + ",\n";
        output += "    \"Nutation and precession matrix row 1 column 1\": " + std::to_string(Orbital_subblock.NPM_R1_C1 * 1e-12) + ",\n";
        output += "    \"Nutation and precession matrix row 2 column 1\": " + std::to_string(Orbital_subblock.NPM_R2_C1 * 1e-14) + ",\n";
        output += "    \"Nutation and precession matrix row 3 column 1\": " + std::to_string(Orbital_subblock.NPM_R3_C1 * 1e-14) + ",\n";
        output += "    \"Nutation and precession matrix row 1 column 2\": " + std::to_string(Orbital_subblock.NPM_R1_C2 * 1e-14) + ",\n";
        output += "    \"Nutation and precession matrix row 2 column 2\": " + std::to_string(Orbital_subblock.NPM_R2_C2 * 1e-12) + ",\n";
        output += "    \"Nutation and precession matrix row 3 column 2\": " + std::to_string(Orbital_subblock.NPM_R3_C2 * 1e-16) + ",\n";
        output += "    \"Nutation and precession matrix row 1 column 3\": " + std::to_string(Orbital_subblock.NPM_R1_C3 * 1e-12) + ",\n";
        output += "    \"Nutation and precession matrix row 2 column 3\": " + std::to_string(Orbital_subblock.NPM_R2_C3 * 1e-16) + ",\n";
        output += "    \"Nutation and precession matrix row 3 column 3\": " + std::to_string(Orbital_subblock.NPM_R3_C3 * 1e-12) + ",\n";
        output += "    \"Sub-satellite North Latitude, [deg]\": " + std::to_string(Orbital_subblock.SSP_LATITUDE * 1e-8) + ",\n";
        output += "    \"Sub-satellite East Longitude, [deg]\": " + std::to_string(Orbital_subblock.SSP_LONGITUDE * 1e-8) + ",\n";
        output += "    \"Satellite altitude above Earth surface [m]\": " + std::to_string(Orbital_subblock.ALTITUDE * 1e-6) + "\n";

        output += "}";
        return output;
    };

} // namespace fengyun_svissr