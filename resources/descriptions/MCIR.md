# MCIR

This enhancement strips only the clouds from the satellite image, and applies them as a layer over a predefined map. This can help to differentiate some cloud types from land in single-band infrared imagery, typical of APT transmissions.

Compared to the Cloud Underlay, it tries to preserve more cloud data.

### Appearance

Clouds appear white, progressively less transparent as the cloud thickens.
Land and sea appear as specified by the map used (by default, a physical map).

### Intended usage

Cloud analysis on APT.

### Limitations

No cloud top temperatures.