# Shortwave Infrared Window - Band 7

According to Lindstrom et al. [1], the 3.9 µm band (Band 7) "can be used to identify fog and low clouds at night, identify fire hot spots, detect volcanic ash, estimate sea-surface temperatures, and discriminate between ice crystal sizes during the day" Lindstrom et al. [1]. Additionally, this band can be used to estimate low-level atmospheric vector winds, as well as to study urban heat islands.

This band is also described as unique by Lindstrom et al. [1] when compared to other ABI bands because it senses emitted terrestrial radiation and significant reflected solar radiation during daytime.

### Intended usage

Primarily used for fire detection because of it's higher sensitivity when compared to longer wavelength infrared channels.

### Appearance

"Small ice crystals reflect more solar 3.9 µm radiation than large crystals during daytime." Lindstrom et al. [1].

"Stratus clouds don't emit 3.9 µm radiation as a blackbody so the inferred temperature is colder than the inferred from the 10.3 µm radiation. Thus, at night, stratus clouds are apparent in the brightness temperature difference." Lindstrom et al. [1].

The temperature range used with the following LUT is 190 K to 400 K.

![Scale](lut/cal/abi_ir_7.png)

### Limitations

As the reflected solar reflectance adds to the detected 3.9 µm radiation, when compared to the 10.3 µm band, brightness temperatures are much warmer.

The 2 km resolution causes small fires to not be detected.

### References

1. S. Lindstrom, T. Schmit and J. Gerth "Band 7 - ABI Quick Information Guide", UW-Madison CIMSS/NOAA, Sept. 2017, https://cimss.ssec.wisc.edu/goes/OCLOFactSheetPDFs/ABIQuickGuide_Band07.pdf. [View Article](https://cimss.ssec.wisc.edu/goes/OCLOFactSheetPDFs/ABIQuickGuide_Band07.pdf)
