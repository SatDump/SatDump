#pragma once

#include <random>
#include "common/dsp/complex.h"

/*
Random generator adapted from GNU Radio
I only use gaussian as of now, but others could come in handy later so...
Better keep them.
*/
namespace dsp
{
   class Random
   {
   private:
      long d_seed;
      bool d_gauss_stored;
      float d_gauss_value;

      std::mt19937 d_rng;                              // mersenne twister as random number generator
      std::uniform_real_distribution<float> d_uniform; // choose uniform distribution, default is [0,1)
      std::uniform_int_distribution<> d_integer_dis;

   public:
      Random(unsigned int seed = 0, int min_integer = 0, int max_integer = 2);
      ~Random();

      /*!
         * \brief Change the seed for the initialized number generator. seed = 0 initializes
         * the random number generator with the system time.
         */
      void reseed(unsigned int seed);

      /*!
         * set minimum and maximum for integer random number generator.
         * Limits are [minimum, maximum)
         * Default: [0, std::numeric_limits< IntType >::max)]
         */
      void set_integer_limits(const int minimum, const int maximum);

      /*!
         * Uniform random integers in the range set by 'set_integer_limits' [min, max).
         */
      int ran_int();

      /*!
         * \brief Uniform random numbers in the range [0.0, 1.0)
         */
      float ran1();

      /*!
         * \brief Normally distributed random numbers (Gaussian distribution with zero mean
         * and variance 1)
         */
      float gasdev();

      /*!
         * \brief Laplacian distributed random numbers with zero mean and variance 1
         */
      float laplacian();

      /*!
         * \brief Rayleigh distributed random numbers (zero mean and variance 1 for the
         * underlying Gaussian distributions)
         */
      float rayleigh();

      /*!
         * \brief Exponentially distributed random numbers with values less than or equal
         * to factor replaced with zero. The underlying exponential distribution has
         * mean sqrt(2) and variance 2.
         */
      float impulse(float factor);

      /*!
         * \brief Normally distributed random numbers with zero mean and variance 1 on real
         * and imaginary part. This results in a Rayleigh distribution for the amplitude and
         * an uniform distribution for the phase.
         */
      complex_t rayleigh_complex();
   };
} // namespace libdsp