#pragma once

#include <complex>

/* 
Simple complex numbers implementation.
While this isn't really required, as std::complex<float>
usually does very fine, some inconsistencies in performance
were observed between platforms.
Hence, seems like providing our own helps a lot more than I
originally expected / tested on my machine.
*/
//namespace dsp
//{
struct complex_t
{
    float real;
    float imag;

    complex_t() : real(0), imag(0) {}
    complex_t(float real) : real(real), imag(0) {}
    complex_t(float real, float imag) : real(real), imag(imag) {}
    complex_t(std::complex<float> v) : real(v.real()), imag(v.imag()) {}
    operator std::complex<float>() const { return std::complex<float>(real, imag); }
    //operator std::complex<float> *() const { return (std::complex<float> *)this; }

    // Complex conjugate
    complex_t conj()
    {
        return complex_t{real, -imag};
    }

    // Normal
    float norm()
    {
        return sqrt(real * real + imag * imag);
    }

    // Complex / Complex operations
    complex_t operator+(const complex_t &b)
    {
        return complex_t(real + b.real, imag + b.imag);
    }

    complex_t &operator+=(const complex_t &b)
    {
        real += b.real;
        imag += b.imag;
        return *this;
    }

    complex_t operator-(const complex_t &b)
    {
        return complex_t(real - b.real, imag - b.imag);
    }

    complex_t &operator-=(const complex_t &b)
    {
        real -= b.real;
        imag -= b.imag;
        return *this;
    }

    complex_t operator*(const complex_t &b)
    {
        return complex_t((real * b.real) - (imag * b.imag),
                         (imag * b.real) + (real * b.imag));
    }

    // Complex / Int
    std::complex<float> operator=(const int &b)
    {
        return std::complex<float>(b, 0);
    }

    // Complex / Float operations
    complex_t operator*(const float &b)
    {
        return complex_t(real * b, imag * b);
    }

    complex_t operator/(const float &b)
    {
        return complex_t(real / b, imag / b);
    }

    complex_t &operator*=(const float &b)
    {
        real *= b;
        imag *= b;
        return *this;
    }
};
//};