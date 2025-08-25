# Projection System

This documentation is also applicable to projection configuretion whilst using it in AngelScript.

## Basic example

This is the simplest projection that will automatically create a GeoTIFF
image (for later use in other software) of the proper size and projected
using the equirectangular projection. It will then be explained further
later.

```json
"Composite Name": {
   "equation": "ch2, ch2, ch1",
   "project": {
      "config": {
         "type": "equirec",
         "auto": true,
         "scalar_x": 0.016,
         "scalar_y": -0.016
      },
      "draw_map_overlay": true,
      "individual_equalize": true,
      "img_format": ".tif"
   }
}
```

### Projection types

`type` :

-   `equirec` (equirectangular)
-   `stereo` (sterographic)
-   `utm` (Universal Transverse Mercator)
-   `geos` (for geostationary satellites)
-   `tpers` (Tilted PERSpective)
-   `webmerc` (Web Mercator)

### Common parameters for all projections

These can be omitted in automatic mode, or can be set anyway to
override. If they are omitted but needed, they default to zero.

 -   `lat0` (float) Latitude of the origin of the projection
 -   `lon0` (float) Longitude of the origin of the projection
 -   `offset_x` (float) Offset of the projection from the center (in
     meters)
 -   `offset_y` (float) Offset of the projection from the center (in
     meters)
 -   `scalar_x` (float) Scale of the projection (in meters per pixel)
 -   `scalar_y` (float) Scale of the projection (in meters per pixel)

### Extra parameters for UTM

-   `zone`: (int) UTM zone
-   `south`: (boolean) if you\'re north (false) or south (true) of the
    equator

### Extra parameters for GEOS

-   `altitude`: (float) Altitude of the satellite (in meters)
-   `sweep_x`: (boolean) Sweep mode of the GEOS projection

### Extra parameters for TPERS

-   `altitude`: (float) Altitude of the satellite (in meters)
-   `tilt`: (float) Tilt of the satellite (in degrees)
-   `azimuth`: (float) Azimuth of the satellite (in degrees)

### Alternative mode for Stereo

Mandatory:

-   `center_lat` (float) Latitude of the center of the projection
    (degrees)
-   `center_lon` (float) Longitude of the center of the projection
    (degrees)
-   `scale` (float) Scale of the image(meters per pixel)
-   `width` (int) Width of the image (in pixels)
-   `height` (int) Height of the image (in pixels)

### Alternative mode for TPERS

Mandatory:

-   `center_lat` (float) Latitude of the center of the projection
    (degrees). Set this to 0 to project a geostationary satellite.
-   `center_lon` (float) Longitude of the center of the projection
    (degrees). Set this to the satellite's longitude (e.g. 76°E for Elektro-L N°3) to project a geostationary satellite.
-   `scale` (float) Scale of the image(meters per pixel)
-   `width` (int) Width of the output image (in pixels) 
-   `height` (int) Height of the output image (in pixels)
-   `altitude`: (float) Altitude of the satellite (in meters). Set this to `36000e3` to have the normal altitude of a geostationary satellite.

Optional:

-   `tilt`: (float) Tilt of the satellite (in degrees)
-   `azimuth`: (float) Azimuth of the satellite (in degrees)

### Alternative mode for UTM

Mandatory:

-   `scale` (float) Scale of the image(meters per pixel)
-   `width` (int) Width of the image (in pixels)
-   `height` (int) Height of the image (in pixels)

Optional:

-   `zone`: (int) UTM zone
-   `south`: (boolean) if you\'re north (false) or south (true) of
    the equator
