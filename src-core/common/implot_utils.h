#pragma once

#include <cmath>
#include <cstdio>

inline int MetricFormatter(double value, char *buff, int size, void *data)
{
    const char *unit = (const char *)data;
    static double v[] = {1000000000, 1000000, 1000, 1, 0.001, 0.000001, 0.000000001};
    static const char *p[] = {"G", "M", "k", "", "m", "u", "n"};
    if (value == 0)
    {
        return snprintf(buff, size, "0 %s", unit);
    }
    for (int i = 0; i < 7; ++i)
    {
        if (fabs(value) >= v[i])
        {
            return snprintf(buff, size, "%g %s%s", value / v[i], p[i], unit);
        }
    }
    return snprintf(buff, size, "%g %s%s", value / v[6], p[6], unit);
}
