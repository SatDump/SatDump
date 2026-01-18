#pragma once

#include <cstdint>

namespace antenna
{
    /*!
     * \brief Parent class for antenna objects.
     * \ingroup antenna
     * \details
     *
     * Parent of an antenna class that will simulate different types
     * of antennas.
     */
    class GenericAntenna
    {
    public:
        static int base_unique_id;

        /*!
         * The enumeration that defines the type of the antenna
         */
        enum antenna_t
        {
            YAGI,
            HELIX,
            PARABOLIC_REFLECTOR,
            CANTED_TURNSTYLE,
            CUSTOM,
            MONOPOLE,
            DIPOLE,
            QUADRIFILAR_HELIX
        };

        uint8_t d_type;

        double d_frequency;
        double d_pointing_error;

        int d_polarization;
        int my_id;

        int unique_id();

        /*!
         * \brief Get the frequency of the antenna.
         * \return the frequency in Hz.
         */
        double get_frequency();

        /*!
         * \brief Set the pointing error of the antenna.
         * \param error the pointing error in degrees.
         */
        void set_pointing_error(double error);

        /*!
         * \brief Get the pointing error of the antenna.
         * \return the pointing error in degrees.
         */
        double get_pointing_error();

        /*!
         * \brief Get the polarization of the antenna.
         * \return the gr::leo::generic_antenna::Polarization enum.
         */
        int get_polarization();

        /*!
         * \brief Get the wavelength of the antenna.
         * \return the wavelength in meters.
         */
        double get_wavelength();

        /*!
         * \brief Get the gain of the antenna. This pure virtual
         * function MUST be implemented by every derived class.
         * \return the gain in dBiC.
         */
        virtual double get_gain() = 0;

        /*!
         * \brief Get the beamwidth of the antenna. This pure virtual
         * function MUST be implemented by every derived class.
         * \return the beamwidth in degrees.
         */
        virtual double get_beamwidth() = 0;

        /*!
         * \brief Get the the gain roll-off of the antenna.
         * \return the gain roll-off in dB.
         */
        virtual double get_gain_rolloff() = 0;

        virtual ~GenericAntenna();

        /*!
         * \brief The constructor of generic_antenna class
         *
         * \param type The enumeration that defines the type of the antenna
         * \param frequency The frequency of the antenna in Hz
         * \param polarization The enumeration that defines the antenna
         * polarization
         *
         * \return a std::shared_ptr to the constructed tracker object.
         */
        GenericAntenna(uint8_t type, double frequency, int polarization, double pointing_error);

        GenericAntenna(void) {};
    };
} // namespace antenna