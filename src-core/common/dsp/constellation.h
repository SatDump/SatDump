#pragma once
#define M_SQRT2 1.41421356237309504880

#include "complex.h"

namespace dsp
{
    class constellation_t
    {
    protected:
        int symbols_count;        // Number of symbols transmitted per modulated sample
        complex_t *constellation; // LUT used for modulation

    public:
        constellation_t(int size)
        {
            constellation = new complex_t[size];
        }
        ~constellation_t()
        {
            delete[] constellation;
        }
        virtual complex_t mod(uint8_t symbol)
        {
            return constellation[symbol];
        };
        virtual uint8_t demod(complex_t sample) = 0;
        virtual uint8_t soft_demod(int8_t *sample) = 0;
    };

    class qpsk_constellation : public constellation_t
    {
    public:
        qpsk_constellation() : constellation_t(4)
        {
            // Default constellation, Gray-Coded
            constellation[0] = complex_t(-M_SQRT2, -M_SQRT2);
            constellation[1] = complex_t(M_SQRT2, -M_SQRT2);
            constellation[2] = complex_t(-M_SQRT2, M_SQRT2);
            constellation[3] = complex_t(M_SQRT2, M_SQRT2);
        }

        uint8_t demod(complex_t sample)
        {
            return 2 * (sample.imag > 0) + (sample.real > 0);
        }

        uint8_t soft_demod(int8_t *sample)
        {
            return 2 * (sample[1] > 0) + (sample[0] > 0);
        }
    };

    class dqpsk_constellation : public constellation_t
    {
    public:
        dqpsk_constellation() : constellation_t(4)
        {
            // Default constellation, NOT Gray-Coded
            constellation[0] = complex_t(+M_SQRT2, +M_SQRT2);
            constellation[1] = complex_t(-M_SQRT2, +M_SQRT2);
            constellation[2] = complex_t(-M_SQRT2, -M_SQRT2);
            constellation[3] = complex_t(+M_SQRT2, -M_SQRT2);
        }

        uint8_t demod(complex_t sample)
        {
            bool a = sample.real > 0;
            bool b = sample.imag > 0;
            if (a)
            {
                if (b)
                    return 0x0;
                else
                    return 0x3;
            }
            else
            {
                if (b)
                    return 0x1;
                else
                    return 0x2;
            }
        }

        uint8_t soft_demod(int8_t *sample)
        {
            bool a = sample[0] > 0;
            bool b = sample[1] > 0;
            if (a)
            {
                if (b)
                    return 0x0;
                else
                    return 0x3;
            }
            else
            {
                if (b)
                    return 0x1;
                else
                    return 0x2;
            }
        }
    };
};