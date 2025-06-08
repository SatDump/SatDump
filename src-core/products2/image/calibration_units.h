#pragma once

/**
 * @file calibration_units.h
 */

#include <map>
#include <string>

// TODOREWORK move all this out of products! / change namespace

#define CALIBRATION_INVALID_VALUE -9999.9

#define CALIBRATION_ID_SUN_ANGLE "sun_angle"
#define CALIBRATION_ID_ALBEDO "albedo"
#define CALIBRATION_ID_SUN_ANGLE_COMPENSATED_ALBEDO "sun_angle_compensated_albedo"
#define CALIBRATION_ID_EMISSIVE_RADIANCE "emissive_radiance"
#define CALIBRATION_ID_REFLECTIVE_RADIANCE "reflective_radiance"
#define CALIBRATION_ID_SUN_ANGLE_COMPENSATED_REFLECTIVE_RADIANCE "sun_angle_compensated_reflective_radiance"
#define CALIBRATION_ID_BRIGHTNESS_TEMPERATURE "brightness_temperature"
#define CALIBRATION_ID_BRIGHTNESS_TEMPERATURE_CELSIUS "brightness_temperature_celsius"
#define CALIBRATION_ID_BACKSCATTER "backscatter"

namespace satdump
{
    namespace calibration
    {
        /**
         * @brief Calibration Unit information
         *
         * @param unit the physical unit of the type
         * @param name readable name of the type
         */
        struct UnitInfo
        {
            std::string unit;
            std::string name;

            /**
             * @brief Return a nicer name
             * @return string of a nice name for UI use
             */
            std::string getNiceUnits() { return name + " (" + unit + ")"; }
        };

        /**
         * @brief Event to let plugins register their own
         * calibration type IDs and associate a readable name
         * and unit.
         *
         * @param r map to push unit ID and their info into
         */
        struct RequestCalibrationUnitsEvent
        {
            std::map<std::string, UnitInfo> &r;
        };

        /**
         * @brief Function to get UnitInfo from an unit
         * ID. Do note this calls plugin events everytime and therefore
         * is not meant to be very optimized.
         *
         * @return the unit's info
         */
        UnitInfo getUnitInfo(std::string id);
    } // namespace calibration
} // namespace satdump