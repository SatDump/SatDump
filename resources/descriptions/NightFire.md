# Night Time Fire Detection

Night Time Fire Detection is a threshold algorithm intended for *fire detection during night time*.
The current algorithm is an adaptation of the *NOCTURNAL THRESHOLD MODEL* by S. Boles and D. L. Verbyla [1].

### Appearance

The fire spots will be displayed as a red mask with an underlying transparent layer, meant to be used on top of other imagery or projected.

![NightFire](descriptions/img/NightFire.png)

### Intended usage

Fire detection for nocturnal imagery.

### Limitations

Other algorithms have a higher percentage of detection [1].

Currently only available for AVHRR-3 imagery.

Only for fire detection during night time imagery.

Bad/noisy data, especially from *NOAA POES*, will give false positives, which can be attenuated by using median blur, although *this is not recommended for general purpose usage*.

Currently tested on data from Brazil.

### References

1. S. Boles and D. L. Verbyla, “Comparison of three AVHRR-Based fire Detection Algorithms for Interior Alaska,” Remote Sensing of Environment, vol. 72, no. 1, pp. 1–16, Apr. 2000, doi: 10.1016/s0034-4257(99)00079-6. [View Article](https://doi.org/10.1016/S0034-4257(99)00079-6)
