# Projection

This aims at editing the auto generated projections and the possible parameters related to it. The goal is not to explain what every option does, but rather allow users to find the right flags in CLI mode.


## General

The settings for the auto generated projections are saved in satdump_cfg.json
Beware that you need to edit /usr/share/satdump/satdump_cfg.json (with su) if you installed SatDump system-wide.

The settings for the different instruments and their composites can be found under: 

```
    "viewer": {
        "instruments": {
        ...
        
        }
    }
```

This is an example how one instrument could be listed:

```
    "viewer": {
        "instruments": {
            ...
            "avhrr_3": {
                "handler": "image_handler",
                "name": "AVHRR/3",
                "rgb_composites": {
                    "221": {
                        "equation": "ch2, ch2, ch1",
                        "geo_correct": true,
                        "project": {
                            "width": 4096,
                            "height": 2048,
                            "old_algo": true,
                            "draw_map": true,
                            "equalize": true,
                            "config": {
                                "type": "equirectangular"
                            }
                        }
                    },
                    ...
                },
                ...
            },
            ...
        }
    }
```

Each RGB composite can be individually projected. There are a few different possible projection types that can be selected:
- Equirectangular
- Mercator
- Stereo
- satellite (Tpers)
- Azimuthal Equidistant

There are a few general settings which all projection types have in common. 
- `width` of the projected image
- `height` of the projected image
- `old_algo`: defines whether to use the old or the new projection algorithm
- `draw_map`: defines whether to draw a map overlay
- `equalize`: image brightness equalisation
- `config`: projection specific parameters

```
    ...
    
    "project": {
        "width": 4096,
        "height": 2048,
        "old_algo": false,
        "draw_map": true,
        "equalize": true,
        "config": {
            ...
        }
    }
```

Depending on the type of projection, there are different parameters which you will need to frame your picture right.
The parameters must be added under `"config":`

## Equirectangular

For `equirectangular` projection, the following parameters are needed (else it will use default parameters):
- `tl_lon`: Top left corner longitude
- `tl_lat`: Top left corner latitude
- `br_lon`: Bottom right corner longitude
- `br_lat`: Bottom right corner latitude

Example:
```
    ...
    "project": {
        "width": 4096,
        "height": 2048,
        "old_algo": false,
        "draw_map": true,
        "equalize": true,
        "config": {
            "type": "equirectangular",
            "tl_lon": -30.0,
            "tl_lat": 80.0,
            "br_lon": 40.0,
            "br_lat": 30.0 
        }
    }
```

## Mercator

There are no additional parameters required for this projection type.

Example:
```
    ...
    "project": {
        "width": 4096,
        "height": 2048,
        "old_algo": false,
        "draw_map": true,
        "equalize": true,
        "config": {
            "type": "mercator"
        }
    }
```

## Stereo

For `stereo` projection, the following parameters are needed (else it will use default parameters):
- `center_lon`: Center longitude
- `center_lat`: Center latitude
- `scale`: Scale

Example:
```
    ...
    "project": {
        "width": 4096,
        "height": 4096,
        "old_algo": false,
        "draw_map": true,
        "equalize": true,
        "config": {
            "type": "stereo",
            "center_lon": 50.0,
            "center_lat": 10.0,
            "scale": 1.0
        }
    }
```

## Satellite (Tpers)

For `tpers` projection, the following parameters are needed (else it will use default parameters):
- `alt`: altitude
- `lon`: longitude
- `lat`: latitude
- `ang`: angle
- `azi`: azimuth

```
    ...
    "project": {
        "width": 4096,
        "height": 4096,
        "old_algo": false,
        "draw_map": true,
        "equalize": true,
        "config": {
            "type": "tpers",
            "alt": 30000.0,
            "lon": 10.0,
            "lat": 50.0,
            "ang": 0.0,
            "azi": 0.0
        }
    }
```

## Azimuthal Equidistant

For `azeq` projection, the following parameters are needed (else it will use default parameters):
- `lon`: longitude
- `lat`: latitude

```
    ...
    "project": {
        "width": 4096,
        "height": 4096,
        "old_algo": false,
        "draw_map": true,
        "equalize": true,
        "config": {
            "type": "azeq",
            "lon": 10.0,
            "lat": 50.0,
        }
    }
```