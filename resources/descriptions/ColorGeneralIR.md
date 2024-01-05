# Color general cloud IR

This enhancement, uses a look-up table to highlight certain temperatures in the clouds.

It greatly enhances the contrast in the land/sea regions with respect to the clouds, which are usually hard to see on pure infrared imagery.

This enhancement uses calibrated temperatures.

### Appearance

Progressively colder clouds appear in shades of yellow, then orange, and then red.

Land and sea appear in shades of blue (cold climates), gray and white (hotter climates).

Freezing point at 273K (0 °C) is materialized by dark blue.

![Scale](descriptions/img/ColorGeneralIRtempscale.png)

Breakpoints at 250K (-23°C), 237K (-36°C), 215K (-58°C) and 200K (-73°C) enable easy and intuitive visualization of the cloud top temperature.

### Intended usage

Meteorology (cloud top measurements)

### Limitations

No specific limitation.