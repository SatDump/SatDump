#include "calibration_units.h"
#include "core/plugin.h"

#define CALIBRATION_RADIANCE_UNIT u8"W\u00B7sr\u207b\u00b9\u00B7m\u207b\u00b2"
#define CALIBRATION_TEMPERATURE_UNIT "°K"
#define CALIBRATION_TEMPERATURE_UNIT_CELSIUS "°C"

namespace satdump
{
    namespace calibration
    {
        std::map<std::string, UnitInfo> unit_registry = {
            {CALIBRATION_ID_SUN_ANGLE, {"Deg", "Sun Angle"}},
            {CALIBRATION_ID_ALBEDO, {"NoUnit", "Albedo"}},
            {CALIBRATION_ID_SUN_ANGLE_COMPENSATED_ALBEDO, {"NoUnit", "Sun Angle Compensated Albedo"}},
            {CALIBRATION_ID_EMISSIVE_RADIANCE, {CALIBRATION_RADIANCE_UNIT, "Emissive Radiance"}},
            {CALIBRATION_ID_REFLECTIVE_RADIANCE, {CALIBRATION_RADIANCE_UNIT, "Reflective Radiance"}},
            {CALIBRATION_ID_SUN_ANGLE_COMPENSATED_REFLECTIVE_RADIANCE, {CALIBRATION_RADIANCE_UNIT, "Sun Angle Compensated Reflective Radiance"}},
            {CALIBRATION_ID_BRIGHTNESS_TEMPERATURE, {CALIBRATION_TEMPERATURE_UNIT, "Brightness Temperature"}},
            {CALIBRATION_ID_BRIGHTNESS_TEMPERATURE_CELSIUS, {CALIBRATION_TEMPERATURE_UNIT_CELSIUS, "Brightness Temperature"}},
            {CALIBRATION_ID_BACKSCATTER, {"Backscatter", "Backscatter"}},
        };

        UnitInfo getUnitInfo(std::string id)
        {
            std::map<std::string, UnitInfo> reg = unit_registry;
            eventBus->fire_event<RequestCalibrationUnitsEvent>({reg});
            if (reg.count(id))
                return reg[id];
            else
                return {"unknown", "Unknown Unit"};
        }
    } // namespace calibration
} // namespace satdump