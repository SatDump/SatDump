#include "random.h"
#include <chrono>

namespace dsp
{
    Random::Random(unsigned int seed, int min_integer, int max_integer)
        : d_rng(), d_integer_dis(0, 1)
    {
        d_gauss_stored = false; // set gasdev (gauss distributed numbers) on calculation state

        // Setup Random number generators
        reseed(seed); // set seed for Random number generator
        set_integer_limits(min_integer, max_integer);
    }

    Random::~Random() {}

    /*
    * Seed is initialized with time if the given seed is 0. Otherwise the seed is taken
    * directly. Sets the seed for the Random number generator.
    */
    void Random::reseed(unsigned int seed)
    {
        d_seed = seed;
        if (d_seed == 0)
        {
            auto now = std::chrono::system_clock::now().time_since_epoch();
            auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
            d_rng.seed(ns);
        }
        else
        {
            d_rng.seed(d_seed);
        }
    }

    void Random::set_integer_limits(const int minimum, const int maximum)
    {
        // boost expects integer limits defined as [minimum, maximum] which is unintuitive.
        // use the expected half open interval behavior! [minimum, maximum)!
        d_integer_dis = std::uniform_int_distribution<>(minimum, maximum - 1);
    }

    /*!
    * Uniform Random integers in the range set by 'set_integer_limits' [min, max).
    */
    int Random::ran_int() { return d_integer_dis(d_rng); }

    /*
    * Returns uniformly distributed numbers in [0,1) taken from boost.Random using a Mersenne
    * twister
    */
    float Random::ran1() { return d_uniform(d_rng); }

    /*
    * Returns a normally distributed deviate with zero mean and variance 1.
    * Used is the Marsaglia polar method.
    * Every second call a number is stored because the transformation works only in pairs.
    * Otherwise half calculation is thrown away.
    */
    float Random::gasdev()
    {
        if (d_gauss_stored)
        { // just return the stored value if available
            d_gauss_stored = false;
            return d_gauss_value;
        }
        else
        { // generate a pair of gaussian distributed numbers
            float x, y, s;
            do
            {
                x = 2.0 * ran1() - 1.0;
                y = 2.0 * ran1() - 1.0;
                s = x * x + y * y;
            } while (s >= 1.0f || s == 0.0f);
            d_gauss_stored = true;
            d_gauss_value = x * sqrtf(-2.0 * logf(s) / s);
            return y * sqrtf(-2.0 * logf(s) / s);
        }
    }

    float Random::laplacian()
    {
        float z = ran1();
        if (z > 0.5f)
        {
            return -logf(2.0f * (1.0f - z));
        }
        return logf(2 * z);
    }
    /*
    * Copied from The KC7WW / OH2BNS Channel Simulator
    * FIXME Need to check how good this is at some point
    */
    // 5 => scratchy, 8 => Geiger
    float Random::impulse(float factor = 5)
    {
        float z = -1.41421356237309504880 * logf(ran1());
        if (fabsf(z) <= factor)
            return 0.0;
        else
            return z;
    }

    complex_t Random::rayleigh_complex() { return complex_t(gasdev(), gasdev()); }

    float Random::rayleigh() { return sqrtf(-2.0 * logf(ran1())); }
} // namespace libdsp