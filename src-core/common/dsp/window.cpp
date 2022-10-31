#include "window.h"
#define M_PI 3.14159265358979323846
#include <cmath>

namespace dsp
{
    namespace window
    {
        double cosine(double n, double N, const double *coefs, int coefCount)
        {
            //  assert(coefCount > 0);
            double win = 0.0;
            double sign = 1.0;
            for (int i = 0; i < coefCount; i++)
            {
                win += sign * coefs[i] * cos((double)i * 2.0 * M_PI * n / N);
                sign = -sign;
            }
            return win;
        }

        double nuttall(double n, double N)
        {
            const double coefs[] = {0.355768, 0.487396, 0.144232, 0.012604};
            return cosine(n, N, coefs, sizeof(coefs) / sizeof(double));
        }
    }

    double sinc(double x)
    {
        return (x == 0.0) ? 1.0 : (sin(x) / x);
    }

    std::vector<float> windowed_sinc(int count, double omega, std::function<double(double, double)> window, double norm)
    {
        // Allocate taps
        std::vector<float> taps = std::vector<float>(count);

        // Generate using window
        double half = (double)count / 2.0;
        double corr = norm * omega / M_PI;

        for (int i = 0; i < count; i++)
        {
            double t = (double)i - half + 0.5;
            taps[i] = sinc(t * omega) * window(t - half, count) * corr;
        }

        return taps;
    }
} // namespace dsp
