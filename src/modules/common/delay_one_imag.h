#include <complex>
#include <vector>

class DelayOneImag
{
public:
    DelayOneImag();
    void work(std::complex<float> *in, size_t length, std::complex<float> *out);

private:
    float lastSamp;
};