#include "generic.h"
#include <cstddef>

namespace antenna
{
    /*!
     * \brief Class that represents a Quadrifilar Helix antenna.
     * \ingroup antenna
     * \details
     *
     * The helix_antenna class extends the generic_antenna class
     * to simulate the behavior of a quadrifilar helix antenna.
     */
    class QuadrifilarHelixAntenna : public GenericAntenna
    {
    public:
        /*!
         * \brief The constructor of quadrifilar_helix_antenna class
         *
         * \param type The enumeration that defines the type of the antenna
         * \param frequency The frequency of the antenna in Hz
         * \param polarization The enumeration that defines the antenna
         * polarization
         * \param pointing_error The pointing error of the antenna in degrees.
         * \param loop The loop of the quadrifilar helix antenna
         */
        QuadrifilarHelixAntenna(uint8_t type, double frequency, int polarization, double pointing_error, double loop);

        ~QuadrifilarHelixAntenna();

        /*!
         * \brief Get the gain of the quadrifilar helix antenna. This is the implementation
         * of the parent's pure virtual function for the quadrifilar helix antenna.
         * \return the gain in dBiC.
         */
        double get_gain();

        /*!
         * \brief Get the the gain roll-off of the antenna.
         * \return the gain roll-off in dB.
         */
        double get_gain_rolloff();

        /*!
         * \brief Get the beamwidth of the quadrifilar helix antenna. This is the implementation
         * of the parent's pure virtual function for the quadrifilar helix antenna.
         * \return the beamwidth.
         */
        double get_beamwidth();

    private:
        double d_loop;
    };
} // namespace antenna