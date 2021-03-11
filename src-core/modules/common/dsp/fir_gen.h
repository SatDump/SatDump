#pragma once

#include <vector>
#include "fft_window.h"

namespace dsp
{
    namespace firgen
    {
        std::vector<float> window(fft::window::win_type type, int ntaps, double beta);

        /*!
     * \brief Use "window method" to design a low-pass FIR filter.  The
     * normalized width of the transition band is what sets the number of
     * taps required.  Narrow --> more taps.  Window type determines maximum
     * attenuation and passband ripple.
     *
     * \param gain                overall gain of filter (typically 1.0)
     * \param sampling_freq       sampling freq (Hz)
     * \param cutoff_freq	        center of transition band (Hz)
     * \param transition_width	width of transition band (Hz)
     * \param window              one of fft::window::win_type
     * \param beta                parameter for Kaiser window
     */
        std::vector<float>
        low_pass(double gain,
                 double sampling_freq,
                 double cutoff_freq,      // Hz center of transition band
                 double transition_width, // Hz width of transition band
                 fft::window::win_type window = fft::window::win_type::WIN_HAMMING,
                 double beta = 6.76); // used only with Kaiser

        /*!
     * \brief design a Root Cosine FIR Filter (do we need a window?)
     *
     * \param gain            overall gain of filter (typically 1.0)
     * \param sampling_freq   sampling freq (Hz)
     * \param symbol_rate     symbol rate, must be a factor of sample rate
     * \param alpha           excess bandwidth factor
     * \param ntaps           number of taps
     */
        std::vector<float>
        root_raised_cosine(double gain,
                           double sampling_freq,
                           double symbol_rate, // Symbol rate, NOT bitrate (unless BPSK)
                           double alpha,       // Excess Bandwidth Factor
                           int ntaps);

        // Hilbert transform
        std::vector<float> hilbert(unsigned int ntaps, fft::window::win_type windowtype, double beta);

        /**
    * @brief Given the interpolation rate, decimation rate and a fractional bandwidth,
     design a set of taps.
    *
    * Uses default parameters to build a low pass filter using a Kaiser Window
     *
    * @param interpolation interpolation factor (integer > 0)
    * @param decimation decimation factor (integer > 0)
    * @param fractional_bw fractional bandwidth in (0, 0.5)  0.4 works well. (float)
    */
        std::vector<float> design_resampler_filter_float(const unsigned interpolation, const unsigned decimation, const float fractional_bw);
    } // namespace firgen
} // namespace libdsp
