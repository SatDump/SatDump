#pragma once

#include <vector>
#include <cstdint>

class NevilleInterpolator
{
private:
    const std::vector<std::pair<double, double>> xy;

    mutable double x;

    double P(size_t i, size_t j)
    {
        if (i == j)
            return xy[i - 1].second;

        double Xi = xy[i - 1].first;
        double Xj = xy[j - 1].first;

        return ((Xj - x) * P(i, j - 1) + (x - Xi) * P(i + 1, j)) / (Xj - Xi);
    }

public:
    NevilleInterpolator(std::vector<std::pair<double, double>> xy)
        : xy(xy)
    {
        std::sort(xy.begin(), xy.end(), [](auto &el1, auto &el2)
                  { return el1.first < el2.first; });
    }

    double interpolate(double xvalue)
    {
        x = xvalue;
        return P(1, xy.size());
    }
};

class LinearInterpolator
{
private:
    const std::vector<std::pair<double, double>> xy;

public:
    LinearInterpolator(std::vector<std::pair<double, double>> xy)
        : xy(xy)
    {
        std::sort(xy.begin(), xy.end(), [](auto &el1, auto &el2)
                  { return el1.first < el2.first; });
    }

    double interpolate(double xvalue)
    {
        int start_pos = 0;
        while (start_pos < (int)xy.size() && xvalue > xy[start_pos].first)
            start_pos++;

        if (start_pos + 1 == (int)xy.size())
            start_pos--;
        if (start_pos == 0)
            start_pos++;

        double x1 = xy[start_pos].first;
        double x2 = xy[start_pos + 1].first;
        double y1 = xy[start_pos].second;
        double y2 = xy[start_pos + 1].second;

        // printf("%d - %f %f %f - %f %f\n", start_pos, x1, xvalue, x2, y1, y2);

        double x_len = x2 - x1;
        xvalue -= x1;

        double y_len = y2 - y1;

        return y1 + (xvalue / x_len) * y_len;
    }
};
