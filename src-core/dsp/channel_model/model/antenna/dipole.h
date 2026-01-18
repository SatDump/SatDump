#include "generic.h"
#include <cstddef>

namespace antenna
{
    /*!
     * \brief Class that represents a dipole antenna.
     * \ingroup antenna
     * \details
     *
     * The dipole_antenna class extends the generic_antenna class
     * to simulate the behavior of a dipole antenna.
     */
    class DipoleAntenna : public GenericAntenna
    {
    public:
        /*!
         * \brief The constructor of dipole_antenna class
         *
         * \param type The enumeration that defines the type of the antenna
         * \param frequency The frequency of the antenna in Hz
         * \param polarization The enumeration that defines the antenna
         * polarization
         * \param pointing_error The pointing error of the antenna in degrees.
         */
        DipoleAntenna(uint8_t type, double frequency, int polarization, double pointing_error);

        ~DipoleAntenna();

        /*!
         * \brief Get the gain of the dipole antenna. This is the implementation
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
         * \brief Get the beamwidth of the dipole antenna. This is the implementation
         * of the parent's pure virtual function for the dipole antenna.
         * \return the beamwidth.
         */
        double get_beamwidth();
    };
} // namespace antenna