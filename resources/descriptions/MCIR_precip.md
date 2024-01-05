# MCIR Rain

This enhancement strips only the clouds from the satellite image, and applies them as a layer over a predefined map. This can help to differentiate some cloud types from land in single-band infrared imagery, typical of APT transmissions.

After that, it applies the NO enhancement on top.

### Appearance

Clouds with progressively higher likelyhood of precipitation, including areas that are already receiving precipitation, appear green, then yellow, then red, and finally black.

Clouds with no precipitation likelyhood are white or grayish.

Land and sea appear as specified by the map used (by default, a physical map).

![NO](lut/cal/WXtoImg-NO.png)

### Intended usage

Cloud analysis on APT.

### Limitations

No specific limitations.