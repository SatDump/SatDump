#pragma once

#include "generic.h"

namespace antenna
{
    /*!
     * \brief Class that represents a parabolic reflector antenna.
     * \ingroup antenna
     * \details
     *
     * The parabolic_reflector_antenna class extends the generic_antenna class
     * to simulate the behavior of a parabolic reflector antenna.
     */
    class ParabolicReflectorAntenna : public GenericAntenna
    {
        /*!
         * \brief The constructor of parabolic_reflector_antenna class
         *
         * \param type The enumeration that defines the type of the antenna
         * \param frequency The frequency of the antenna in Hz
         * \param polarization The enumeration that defines the antenna
         * polarization
         * \param pointing_error The pointing error of the antenna in degrees.
         * \param diameter The diameter of the antenna in meters.
         * \param aperture_efficiency The aperture_efficiency efficiency.
         *
         */
    public:
        ParabolicReflectorAntenna(uint8_t type, double frequency, int polarization, double pointing_error, double diameter, double aperture_efficiency);

        ~ParabolicReflectorAntenna();

        /*!
         * \brief Get the gain of the parabolic reflector antenna. This is the implementation
         * of the parent's pure virtual function for the parabolic reflector antenna.
         * \return the gain in dBiC.
         */
        double get_gain();

        /*!
         * \brief Get the beamwidth of the parabolic reflector antenna. This is the implementation
         * of the parent's pure virtual function for the parabolic reflector antenna.
         * \return the beamwidth
         */
        double get_beamwidth();

        /*!
         * \brief Get the the gain roll-off of the antenna.
         * \return the gain roll-off in dB.
         */
        double get_gain_rolloff();

    private:
        double d_diameter;
        double d_aperture_efficiency;
    };
} // namespace antenna