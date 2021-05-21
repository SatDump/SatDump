#include <complex>
#include <vector>

class DelayOneImag
{
public:
    DelayOneImag();
    void work(std::complex<float> *in, int length, std::complex<float> *out);

private:
    float lastSamp;
};