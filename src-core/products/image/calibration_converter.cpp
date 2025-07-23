#include "calibration_converter.h"
#include "core/plugin.h"
#include "logger.h"
#include "products/image/calibration_units.h"
#include "products/image/converters/kelvin_celcius.h"
#include "products/image_product.h"

#include "converters/bright_temp_to_em_rad.h"
#include "converters/em_rad_to_bright_temp.h"
#include "converters/ref_rad_to_sun_cor_ref_rad.h"
#include "converters/sun_angle.h"

namespace satdump
{
    namespace calibration
    {
        double ConverterBase::convert(const UnitConverter *c, double x, double y, double val) { throw satdump_exception("Function not implemented!"); }
        bool ConverterBase::convert_range(const UnitConverter *c, double &min, double &max) { throw satdump_exception("Function not implemented!"); }

        void UnitConverter::set_proj(nlohmann::json p)
        {
            proj = p;
            proj_valid = proj.init(0, 1);
        }

        void UnitConverter::set_wavenumber(double w) { wavenumber = w; }

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

        UnitConverter::UnitConverter(void *product, std::string channel_name)
        {
            set_proj(((products::ImageProduct *)product)->get_proj_cfg(((products::ImageProduct *)product)->get_channel_image(channel_name).abs_index));
            set_wavenumber(((products::ImageProduct *)product)->get_channel_image(channel_name).wavenumber);
        }

        std::vector<std::shared_ptr<ConverterBase>> getAvailableConverters(std::string itype, std::string otype)
        {
            std::vector<std::shared_ptr<ConverterBase>> converters;

            if (itype == CALIBRATION_ID_REFLECTIVE_RADIANCE && otype == CALIBRATION_ID_SUN_ANGLE_COMPENSATED_REFLECTIVE_RADIANCE)
                converters.push_back(std::make_shared<conv::RefRadToSunCorRefRadConverter>());
            else if (itype == CALIBRATION_ID_EMISSIVE_RADIANCE && otype == CALIBRATION_ID_BRIGHTNESS_TEMPERATURE)
                converters.push_back(std::make_shared<conv::EmRadToBrightTempConverter>(false));
            else if (itype == CALIBRATION_ID_EMISSIVE_RADIANCE && otype == CALIBRATION_ID_BRIGHTNESS_TEMPERATURE_CELSIUS)
                converters.push_back(std::make_shared<conv::EmRadToBrightTempConverter>(true));
            else if (itype == CALIBRATION_ID_BRIGHTNESS_TEMPERATURE && otype == CALIBRATION_ID_EMISSIVE_RADIANCE)
                converters.push_back(std::make_shared<conv::BrightTempToEmRadConverter>());
            else if (itype == CALIBRATION_ID_ALBEDO && otype == CALIBRATION_ID_SUN_ANGLE_COMPENSATED_ALBEDO)
                converters.push_back(std::make_shared<conv::RefRadToSunCorRefRadConverter>());
            else if (itype == CALIBRATION_ID_BRIGHTNESS_TEMPERATURE && otype == CALIBRATION_ID_BRIGHTNESS_TEMPERATURE_CELSIUS)
                converters.push_back(std::make_shared<conv::KelvinCelsiusConverter>(true));
            else if (itype == CALIBRATION_ID_BRIGHTNESS_TEMPERATURE_CELSIUS && otype == CALIBRATION_ID_BRIGHTNESS_TEMPERATURE)
                converters.push_back(std::make_shared<conv::KelvinCelsiusConverter>(false));
            else if (otype == CALIBRATION_ID_SUN_ANGLE)
                converters.push_back(std::make_shared<conv::SunAngleConverter>());

            eventBus->fire_event<ConverterRequestEvent>({converters, itype, otype});

            return converters;
        }

        std::vector<std::string> getAvailableConversions(std::string itype)
        {
            std::vector<std::string> otypes = {itype};

            // Only leave default unit if it actually is valid!
            if (itype == "")
                otypes.clear();

            // logger->trace("Unit Conversions Request : " + itype);

            if (itype == CALIBRATION_ID_REFLECTIVE_RADIANCE)
                otypes.push_back(CALIBRATION_ID_SUN_ANGLE_COMPENSATED_REFLECTIVE_RADIANCE);
            else if (itype == CALIBRATION_ID_EMISSIVE_RADIANCE)
            {
                otypes.push_back(CALIBRATION_ID_BRIGHTNESS_TEMPERATURE);
                otypes.push_back(CALIBRATION_ID_BRIGHTNESS_TEMPERATURE_CELSIUS);
            }
            else if (itype == CALIBRATION_ID_BRIGHTNESS_TEMPERATURE)
            {
                otypes.push_back(CALIBRATION_ID_EMISSIVE_RADIANCE);
                otypes.push_back(CALIBRATION_ID_BRIGHTNESS_TEMPERATURE_CELSIUS);
            }
            else if (itype == CALIBRATION_ID_ALBEDO)
                otypes.push_back(CALIBRATION_ID_SUN_ANGLE_COMPENSATED_ALBEDO);

            otypes.push_back(CALIBRATION_ID_SUN_ANGLE);

            eventBus->fire_event<ConversionRequestEvent>({itype, otypes});

            return otypes;
        }
    } // namespace calibration
} // namespace satdump