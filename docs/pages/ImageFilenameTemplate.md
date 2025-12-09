# Image Filename Template

This box allows the user to customise the proposed file name when saving a new image, or when SatDump automatically saves an image.

There are five presets:

* Default: `avhrr_3_AVHRR_124_False_Color_(Cloud_RGB)`
* Detailed: `avhrr_3_2025-01-11_10-51-19Z_AVHRR_124_False_Color_(Cloud_RGB)`
* Compact Detailed: `202501111041_NOAA_18_AVHRR_124_FALSE_COLOR_(CLOUD_RGB)`
* Satellite and Time: `2025-01-11_1045_UTC_-_NOAA_18`
* WMO: `Z_SATDUMP_C_----_20250111104431_NOAA_18_AVHRR_3_AVHRR124FCCRGB`

## Custom mode

The custom mode allows the creation of custom file names based on tokens. Other characters can also be input in between the tokens, and spaces will be replaced by underscores (`_`).

The meaning of each token is as follows: 

* `$t`: timestamp in Unix format: `1736592312`
* `$y`: year: `2025`
* `$M`: month: `01`
* `$d`: day: `11`
* `$h`: hour: `10`
* `$m`: minute: `45`
* `$s`: second: `12`
* `$i`: instrument (lowercase): `avhrr_3`
* `$I`: instrument (uppercase): `AVHRR_3`
* `$c`: composite/product as specified in the config: `3.75um_IR_(Rainbow)`
* `$C`: composite/product in uppercase: `3.75uM_IR_(RAINBOW)`
* `$a`: abbreviated composite/product name: `3.75IRR`
* `$o`: spacecraft name: `Meteor-M N°2`
* `$O`: spacecraft name (uppercase): `METEOR-M N2`
