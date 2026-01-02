#include "generic.h"
#include <cstddef>

namespace antenna
{
    /*!
     * \brief Class that represents a helix antenna.
     * \details
     *
     * The helix_antenna class extends the generic_antenna class
     * to simulate the behavior of a helix antenna.
     */
    class HelixAntenna : public GenericAntenna
    {
    public:
        /*!
         * \brief The constructor of helix_antenna class
         *
         * \param type The enumeration that defines the type of the antenna
         * \param frequency The frequency of the antenna in Hz
         * \param polarization The enumeration that defines the antenna
         * polarization
         * \param pointing_error The pointing error of the antenna in degrees.
         * \param turns The number of turns
         * \param turn_spacing The spacing of the turns
         * \param circumference The circumference
         *
         */
        HelixAntenna(uint8_t type, double frequency, int polarization, double pointing_error, size_t turns, double turn_spacing, double circumference);

        ~HelixAntenna();

        /*!
         * \brief Get the gain of the helix antenna. This is the implementation
         * of the parent's pure virtual function for the helix antenna.
         * \return the gain in dBiC.
         */
        double get_gain();

        /*!
         * \brief Get the the gain roll-off of the antenna.
         * \return the gain roll-off in dB.
         */
        double get_gain_rolloff();

        /*!
         * \brief Get the beamwidth of the helix antenna. This is the implementation
         * of the parent's pure virtual function for the helix antenna.
         * \return the beamwidth.
         */
        double get_beamwidth();

    private:
        size_t d_turns;
        double d_turn_spacing;
        double d_circumference;
    };
} // namespace antenna