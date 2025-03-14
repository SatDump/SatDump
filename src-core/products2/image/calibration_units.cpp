#include "calibration_units.h"
#include "core/exception.h"
#include "logger.h"
#include "core/plugin.h"

#define CALIBRATION_RADIANCE_UNIT "W\u00B7sr\u207b\u00b9\u00B7m\u207b\u00b2"
#define CALIBRATION_TEMPERATURE_UNIT "°K"

namespace satdump
{
    namespace calibration
    {
        std::map<std::string, UnitInfo> unit_registry = {
            {CALIBRATION_ID_ALBEDO, {"NoUnit", "Albedo (TMP)"}},
            {CALIBRATION_ID_SUN_ANGLE_COMPENSATED_ALBEDO, {"NoUnit", "Sun Angle Compensated Albedo (TMP)"}},
            {CALIBRATION_ID_EMISSIVE_RADIANCE, {CALIBRATION_RADIANCE_UNIT, "Emissive Radiance"}},
            {CALIBRATION_ID_REFLECTIVE_RADIANCE, {CALIBRATION_RADIANCE_UNIT, "Reflective Radiance"}},
            {CALIBRATION_ID_SUN_ANGLE_COMPENSATED_REFLECTIVE_RADIANCE, {CALIBRATION_RADIANCE_UNIT, "Sun Angle Compensated Reflective Radiance"}},
            {CALIBRATION_ID_BRIGHTNESS_TEMPERATURE, {CALIBRATION_TEMPERATURE_UNIT, "Brightness Temperature"}},
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

        double ConverterBase::convert(const UnitConverter *c, double x, double y, double val)
        {
            throw satdump_exception("Function not implemented!");
        }

        void UnitConverter::set_proj(nlohmann::json p)
        {
            proj = p;
            proj.init(0, 1);
        }

        void UnitConverter::set_wavenumber(double w)
        {
            wavenumber = w;
        }

        void UnitConverter::set_conversion(std::string itype, std::string otype)
        {
            converter.reset();

            unitEqual = itype == otype;
            if (unitEqual)
                return;

            auto converters = getAvailableConverters(itype, otype);
            if (converters.size() > 0)
            {
                converter = converters[0];
                return;
            }
            else
            {
                logger->error("No valid converter for : " + itype + " => " + otype);
            }
        }

        //////////////////////////////////////

        namespace
        { // TODOREWORK move to their own files!
            class RefRadToSunCorRefRadConverter : public ConverterBase
            {
            public:
                double convert(const UnitConverter *c, double x, double y, double val)
                {
                    geodetic::geodetic_coords_t pos;
                    double timestamp = -1;
                    if (((UnitConverter *)c)->proj.inverse(x, y, pos, &timestamp))
                        return CALIBRATION_INVALID_VALUE;

                    if (timestamp == -1)
                        return CALIBRATION_INVALID_VALUE;

                    return compensate_radiance_for_sun(val, timestamp, pos.lat, pos.lon);
                }
            };

            class EmRadToBrightTempConverter : public ConverterBase
            {
            public:
                double convert(const UnitConverter *c, double x, double y, double val)
                {
                    if (c->wavenumber == -1)
                        return CALIBRATION_INVALID_VALUE;
                    return radiance_to_temperature(val, c->wavenumber);
                }
            };
        }

        std::vector<std::shared_ptr<ConverterBase>> getAvailableConverters(std::string itype, std::string otype)
        {
            std::vector<std::shared_ptr<ConverterBase>> converters;

            if (itype == CALIBRATION_ID_REFLECTIVE_RADIANCE && otype == CALIBRATION_ID_SUN_ANGLE_COMPENSATED_REFLECTIVE_RADIANCE)
                converters.push_back(std::make_shared<RefRadToSunCorRefRadConverter>());
            else if (itype == CALIBRATION_ID_EMISSIVE_RADIANCE && otype == CALIBRATION_ID_BRIGHTNESS_TEMPERATURE)
                converters.push_back(std::make_shared<EmRadToBrightTempConverter>());
            else if (itype == CALIBRATION_ID_ALBEDO && otype == CALIBRATION_ID_SUN_ANGLE_COMPENSATED_ALBEDO)
                converters.push_back(std::make_shared<RefRadToSunCorRefRadConverter>()); // TODOREWORK? Keep?

            eventBus->fire_event<ConverterRequestEvent>({converters, itype, otype});

            return converters;
        }

        std::vector<std::string> getAvailableConversions(std::string itype)
        {
            std::vector<std::string> otypes = {itype};

            logger->trace("Unit Conversions Request : " + itype);

            if (itype == CALIBRATION_ID_REFLECTIVE_RADIANCE)
                otypes.push_back(CALIBRATION_ID_SUN_ANGLE_COMPENSATED_REFLECTIVE_RADIANCE);
            else if (itype == CALIBRATION_ID_EMISSIVE_RADIANCE)
                otypes.push_back(CALIBRATION_ID_BRIGHTNESS_TEMPERATURE);
            else if (itype == CALIBRATION_ID_ALBEDO)
                otypes.push_back(CALIBRATION_ID_SUN_ANGLE_COMPENSATED_ALBEDO);

            eventBus->fire_event<ConversionRequestEvent>({itype, otypes});

            return otypes;
        }
    }
}