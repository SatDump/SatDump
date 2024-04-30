# Upper-level water vapor - Band 8

The 6.2 µm band (Band 8) is one of 3 water vapor bands present on the ABI. It's "used for tracking upper-tropospheric winds, identifying jet streams, forecasting hurricane track and mid-latitude storm motion, monitoring severe weather potential, estimating upper/mid-level moisture (for legacy vertical moisture profiles) and identifying regions where the potential for turbulance exists" Bachmeier et al. [1].

"It can be used to validate numerical model initialization and warming/cooling with time can reveal vertical motions at mid and upper levels." Bachmeier et al. [1].

### Intended usage

"Atmospheric feature identification (jet streams, troughts/ridges, signatures of potential turbulance)" Bachmeier et al. [1].

Additionally, can identify cloudless features that will shortly after produce clouds/preciptation.


### Appearance

![Interpretation](descriptions/img/ABIUpperLevelWater.png)

The temperature range used with the following LUT is 180 K to 280 K.

![Scale](lut/cal/abi_wv_8-10.png)


### Limitations

"Optically dense clouds obstruct the view of lower altitude moisture features." Bachmeier et al. [1].

As described by Bachmeier et al. [1], the water vapor bands are infrared bands that sense the mean temperature of a layer of moisture, said layer can vary in altitude and depth, depending on the temperature and moisture profile of said atmospheric column, as well as the satellite viewing angle.

### References

1. S. Bachmeier, T. Schmit and J. Gerth "Band 13 - ABI Quick Information Guide", UW-Madison CIMSS/NOAA, Aug. 2017, https://cimss.ssec.wisc.edu/goes/OCLOFactSheetPDFs/ABIQuickGuide_Band13.pdf. [View Article](https://cimss.ssec.wisc.edu/goes/OCLOFactSheetPDFs/ABIQuickGuide_Band13.pdf)
