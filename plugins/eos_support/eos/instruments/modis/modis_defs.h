#pragma once

#define NUM_250M_BANDS 2
#define NUM_500M_BANDS 5
#define NUM_1000M_REFL_BANDS 15
#define NUM_REFLECTIVE_BANDS 22
#define NUM_1000M_EMISS_BANDS 16
#define NUM_EMISSIVE_BANDS 16

#define DETECTORS_PER_1KM_BAND 10
#define DETECTORS_PER_500M_BAND 20
#define DETECTORS_PER_250M_BAND 40
#define NUM_MIRROR_SIDES 2

#define MODIS_BAND26_INDEX_AT_RES 6

#define EV_1km_FRAMES 1354

#define EVAL_2ND_ORDER_POLYNOMIAL(p, a, x, y) p = a[0] + x * a[1] + y * a[2];

#define RVS_CORRECTION_LOWER_LIMIT 4.0e-1
#define RVS_CORRECTION_UPPER_LIMIT 2.4e+0

#define SATURATED_DN 4095

#define CONVERT_TEMP_POLY(tdk, tdn, c, dnsat, toffset) \
    if (tdn > 0 && tdn < dnsat)                        \
        tdk = toffset + c[0] + tdn * (c[1] + tdn * (c[2] + tdn * (c[3] + tdn * (c[4] + tdn * c[5]))));