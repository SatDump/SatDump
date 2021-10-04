#include "maidenhead.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C"
{
#endif
    char letterize(int x)
    {
        return (char)x + 65;
    }

    char *get_mh(double lat, double lon, int size)
    {
        static char locator[11];
        double LON_F[] = {20, 2.0, 0.083333, 0.008333, 0.0003472083333333333};
        double LAT_F[] = {10, 1.0, 0.0416665, 0.004166, 0.0001735833333333333};
        int i;
        lon += 180;
        lat += 90;

        if (size <= 0 || size > 10)
            size = 6;
        size /= 2;
        size *= 2;

        for (i = 0; i < size / 2; i++)
        {
            if (i % 2 == 1)
            {
                locator[i * 2] = (char)(lon / LON_F[i] + '0');
                locator[i * 2 + 1] = (char)(lat / LAT_F[i] + '0');
            }
            else
            {
                locator[i * 2] = letterize((int)(lon / LON_F[i]));
                locator[i * 2 + 1] = letterize((int)(lat / LAT_F[i]));
            }
            lon = fmod(lon, LON_F[i]);
            lat = fmod(lat, LAT_F[i]);
        }
        locator[i * 2] = 0;
        return locator;
    }

    char *complete_mh(char *locator)
    {
        static char locator2[11] = "LL55LL55LL";
        int len = strlen(locator);
        if (len >= 10)
            return locator;
        memcpy(locator2, locator, strlen(locator));
        return locator2;
    }

    double mh2lon(char *locator)
    {
        double field, square, subsquare, extsquare, precsquare;
        int len = strlen(locator);
        if (len > 10)
            return 0;
        if (len < 10)
            locator = complete_mh(locator);
        field = (locator[0] - 'A') * 20.0;
        square = (locator[2] - '0') * 2.0;
        subsquare = (locator[4] - 'A') / 12.0;
        extsquare = (locator[6] - '0') / 120.0;
        precsquare = (locator[8] - 'A') / 2880.0;
        return field + square + subsquare + extsquare + precsquare - 180;
    }

    double mh2lat(char *locator)
    {
        double field, square, subsquare, extsquare, precsquare;
        int len = strlen(locator);
        if (len > 10)
            return 0;
        if (len < 10)
            locator = complete_mh(locator);
        field = (locator[1] - 'A') * 10.0;
        square = (locator[3] - '0') * 1.0;
        subsquare = (locator[5] - 'A') / 24.0;
        extsquare = (locator[7] - '0') / 240.0;
        precsquare = (locator[9] - 'A') / 5760.0;
        return field + square + subsquare + extsquare + precsquare - 90;
    }
#ifdef __cplusplus
}
#endif
