# Cloud Underlay

This enhancement strips only the clouds from the satellite image, and applies them as a layer over a predefined map. This can help to differentiate some cloud types from land in single-band visible imagery, typical of APT transmissions.

Compared to the Cloud Underlay, it tries to preserve more cloud data.

Compared to MCIR, it uses a visible channel (channels 1 or 2 on AVHRR).

### Appearance

Clouds appear white, progressively less transparent as the cloud thickens.
Land and sea appear as specified by the map used (by default, a physical map).

### Intended usage

Aesthetical reasons, cloud analysis on APT at daytime.

### Limitations

No cloud top temperatures.
Not usable at night.