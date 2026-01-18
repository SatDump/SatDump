#pragma once

#include <cstdint>

/*!
 * \brief Parent class for attenuation objects.
 *
 * \details This is an abstract class that must be derived by
 * other classes in order to simulate a specific type of attenuation.
 * Holds information, in the form of static variables, about the operating frequency,
 * the elevation angle, the slant range and the polarization of the tracker
 * at a specific time instance.
 *
 * Each derived class must implement the pure virtual function get_attenuation
 * according to the attenuation it describes, using the static variables of the
 * parent class.
 *
 */
class GenericAttenuation
{
public:
    /*!
     * \brief Get the estimated attenuation. This pure virtual
     * function MUST be implemented by every derived class.
     * \return the attenuation in dB.
     */
    virtual double get_attenuation() = 0;

    /*!
     * \brief Set frequency static variable
     * \param freq The frequency in Hz
     */
    void set_frequency(double freq);

    /*!
     * \brief Set slant range static variable
     * \param range The range in km
     */
    void set_slant_range(double range);

    /*!
     * \brief Set polarization static variable
     * \param polar The polarization enumeration
     */
    void set_polarization(uint8_t polar);

    /*!
     * \brief Set elevation angle static variable
     * \param elev_angle The elevation angle in radians
     */
    void set_elevation_angle(double elev_angle);

    GenericAttenuation();

    virtual ~GenericAttenuation();

protected:
    double frequency = 0;
    double elevation_angle = 0;
    double slant_range = 0;
    uint8_t polarization = 0;
};