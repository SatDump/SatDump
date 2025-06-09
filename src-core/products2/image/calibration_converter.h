#pragma once

/**
 * @file calibration_converter.h
 */

#include "calibration_units.h"

#include "common/calibration.h"
#include "projection/projection.h"

namespace satdump
{
    namespace calibration
    {
        class UnitConverter; // Fwd declaration

        /**
         * @brief Base class for unit converters. This is meant to
         * implement conversions between calibration unit types, for
         * example from emissive radiance measured by a radiometer to
         * brightness temperature, or to apply other processing such
         * as compensating for sun angle in reflective channels, etc.
         */
        class ConverterBase
        {
        public:
            /**
             * @brief Core function (for images!) implementing
             * conversion between calibration unit types.
             *
             * @param c reference to the UnitConverter this belongs to
             * @param x x position in the image
             * @param y position in the image
             * @param val input calibrated value
             * @return converted unit, or CALIBRATION_INVALID_VALUE on errors
             */
            virtual double convert(const UnitConverter *c, double x, double y, double val);

            /**
             * @brief Convert unit ranges. This does the same
             * as convert, except it's optimized for converting
             * a range.
             *
             * @param c reference to the UnitConverter this belongs to
             * @param min min of the range
             * @param max max of the range
             * @return true if the convertion was done
             */
            virtual bool convert_range(const UnitConverter *c, double &min, double &max);
        };

        /**
         * @brief Event to let plugins register their own
         * unit converters.
         *
         * @param c vector to return valid converters
         * @param itype wanted input unit
         * @param otype wanted output unit
         */
        struct ConverterRequestEvent
        {
            std::vector<std::shared_ptr<ConverterBase>> &c;
            std::string itype;
            std::string otype;
        };

        /**
         * @brief Request a converter from a specific unit to another.
         * May return empty vectors, if so no converter was found.
         *
         * @param itype wanted input unit
         * @param otype wanted output unit
         * @return vector of valid converters, should never more than
         * one in theory however!
         */
        std::vector<std::shared_ptr<ConverterBase>> getAvailableConverters(std::string itype, std::string otype);

        /**
         * @brief Event to let plugins list available
         * conversions. Do note it's still possible no
         * converter is provided despite this listing it
         * as available. Shouldn't happen, but do check
         * availability yourself later on.
         *
         * @param c vector to return valid converters
         * @param itype wanted input unit
         * @param otype wanted output unit
         */
        struct ConversionRequestEvent
        {
            std::string itype;
            std::vector<std::string> &otypes;
        };

        /**
         * @brief Get a list of calibration types that can
         * be produced from this unit ID.
         *
         * @param itype input unit
         * @return list of unit IDs you can convert itype to
         */
        std::vector<std::string> getAvailableConversions(std::string itype);

        /**
         * @brief Universal UnitConverter class, handling all
         * conversion logic between any calibration unit type
         * to another.
         *
         * This will automatically setup a conversion if available
         * with the right backend ConverterBase depending on what
         * metadata is available.
         */
        class UnitConverter
        {
        public:
            projection::Projection proj;
            bool proj_valid = false;
            double wavenumber = -1;

        private:
            bool unitEqual = false;
            std::shared_ptr<ConverterBase> converter;

        public:
            /**
             * @brief Simple constructor
             */
            UnitConverter() {}

            /**
             * @brief Setup from an ImageProduct
             *
             * @param product ImageProduct
             * @param channel_name name ID of the channel to use
             */
            UnitConverter(void *product, std::string channel_name);

            /**
             * @brief Sets the projection to utilize for conversion
             * if needed. This is optional, but some conversions will
             * fail if not present. If available, this should be set.
             *
             * This may also be needed to obtain timestamps, if not
             * just projection information.
             */
            void set_proj(nlohmann::json p);

            /**
             * @brief Sets the wavenumber to utilize for conversion
             * if needed. This is optional, but some conversions will
             * fail if not present. If available, this should be set.
             */
            void set_wavenumber(double w);

            /**
             * @brief Sets the conversion this UnitConverter should perform.
             *
             * @param itype input type ID
             * @param otype output type ID
             */
            void set_conversion(std::string itype, std::string otype);

            /**
             * @brief Generic conversion function mostly
             * aimed at converting data from images (TODOREWORK?).
             *
             * @param x position in image
             * @param y position in image
             * @param val input value
             * @return converted value, or CALIBRATION_INVALID_VALUE on error
             */
            inline double convert(double x, double y, double val)
            {
                if (unitEqual)
                    return val;
                else if (converter)
                    return converter->convert(this, x, y, val);
                else
                    return CALIBRATION_INVALID_VALUE;
            }

            /**
             * @brief Convert unit ranges. This does the same
             * as convert, except it's optimized for a range.
             *
             * @param min min of the range
             * @param max max of the range
             * @return true if the convertion was done
             */
            bool convert_range(double &min, double &max)
            {
                if (unitEqual)
                    return true;
                else if (converter)
                    return converter->convert_range(this, min, max);
                else
                    return false;
            }
        };
    } // namespace calibration
} // namespace satdump