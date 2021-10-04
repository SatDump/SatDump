#ifndef MAIDENHEAD_H
#define MAIDENHEAD_H

#ifdef __cplusplus
extern "C"
{
#endif
    char *get_mh(double lat, double lon, int size);
    char *complete_mh(char *locator);
    double mh2lon(char *locator);
    double mh2lat(char *locator);

#ifdef __cplusplus
}
#endif
#endif
