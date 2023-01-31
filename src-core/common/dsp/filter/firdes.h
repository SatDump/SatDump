#pragma once

#include <vector>
#include "common/dsp/complex.h"

/*
Window and FIR generating functions
Taken from GNU Radio
*/
namespace dsp
{
    namespace fft
    {
        class window
        {
        public:
            enum win_type
            {
                WIN_NONE = -1,           //!< don't use a window
                WIN_HAMMING = 0,         //!< Hamming window; max attenuation 53 dB
                WIN_HANN = 1,            //!< Hann window; max attenuation 44 dB
                WIN_BLACKMAN = 2,        //!< Blackman window; max attenuation 74 dB
                WIN_RECTANGULAR = 3,     //!< Basic rectangular window; max attenuation 21 dB
                WIN_KAISER = 4,          //!< Kaiser window; max attenuation see window::max_attenuation
                WIN_BLACKMAN_hARRIS = 5, //!< Blackman-harris window; max attenuation 92 dB
                WIN_BLACKMAN_HARRIS = 5, //!< alias to WIN_BLACKMAN_hARRIS for capitalization consistency
                WIN_BARTLETT = 6,        //!< Barlett (triangular) window; max attenuation 26 dB
                WIN_FLATTOP = 7,         //!< flat top window; useful in FFTs; max attenuation 93 dB
            };

            // GENERATORS
            static std::vector<float> coswindow(int ntaps, float c0, float c1, float c2);
            static std::vector<float> coswindow(int ntaps, float c0, float c1, float c2, float c3);
            static std::vector<float> coswindow(int ntaps, float c0, float c1, float c2, float c3, float c4);
            static std::vector<float> rectangular(int ntaps);
            static std::vector<float> hamming(int ntaps);
            static std::vector<float> hann(int ntaps);
            static std::vector<float> blackman(int ntaps);
            static std::vector<float> blackman_harris(int ntaps, int atten = 92);
            static std::vector<float> kaiser(int ntaps, double beta);
            static std::vector<float> bartlett(int ntaps);
            static std::vector<float> flattop(int ntaps);

            // UTILS
            static double max_attenuation(win_type type, double beta = 6.76);
            static std::vector<float> build(win_type type, int ntaps, double beta, const bool normalize = false);
        };
    };

    namespace firdes
    {
        // Convolve 2 vectors
        std::vector<float> convolve(std::vector<float> u, std::vector<float> v);

        /*!
         * \brief design a Root Cosine FIR Filter.
         *
         * \param gain            overall gain of filter (typically 1.0)
         * \param sampling_freq   sampling freq (Hz)
         * \param symbol_rate     symbol rate, must be a factor of sample rate
         * \param alpha           excess bandwidth factor
         * \param ntaps           number of taps
         */
        std::vector<float> root_raised_cosine(double gain, double sampling_freq, double symbol_rate, double alpha, int ntaps);

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
        std::vector<float> low_pass(double gain, double sampling_freq, double cutoff_freq, double transition_width, fft::window::win_type window = fft::window::win_type::WIN_HAMMING, double beta = 6.76);

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

        /*!
         * \brief design a Gaussian filter
         *
         * \param gain    overall gain of filter (typically 1.0)
         * \param spb     symbol rate, must be a factor of sample rate
         * \param bt      bandwidth to bitrate ratio
         * \param ntaps   number of taps
         */
        std::vector<float> gaussian(double gain,
                                    double spb,
                                    double bt, // Bandwidth to bitrate ratio
                                    int ntaps);
    };
};