# Quick Start

This page is designed to explain the basic operations and controls needed for basic tasks. Its aim is not to explain every single function in detail, but rather give a quick overview, both for new users as well as for users already experienced with legacy SatDump versions.

## Notes for users of previous SatDump versions below 2.0.0 (Legacy SatDump)

* `Recorder` tab moved to its own item (added via the `Recorder` button or with `Add -> Recorder`)
* `Offline processing` tab moved to its own item (`File -> Processing` or with the `Processing` button). Multiple `Processing` tasks can be performed at once.
* `Viewer` tab removed. Satellite data appears under the `Root`.
* `Projections` changed.
* `Settings` tab moved to a floating window accessed with `File -> Settings`.

## Welcome screen

![Welcome screen](quick_start/splash.png)

The *Welcome screen* provides a way to quickly access frequently used functions, as well as provide useful tips on how to best use the software. It is organized in several sections:

### Menu bar

This provides controls to open files, add new tools, change settings and get help. Some menus, such as the `Handler` menu, are contextual and change depending on which item is selected in the `Root`.

### Root

This section collects and organizes hierarchically all the modules, tools and data the user adds to the SatDump section.

### Trash

Dragging any element from the `Root` and dropping it on the `Trash` will delete that element.

### Start Processing button

This button will open the `Processing` panel. This is the same as the old `Offline Processing` tab in legacy SatDump, and it is used to decode satellites from a baseband, frames or other prerecorded files.

### Add Recorder button

This button will add a new `Recorder` to the `Root`. It is the same as the old `Recorder` tab in legacy SatDump, and it is used to record new satellites from a software-defined radio, either directly connected to the computer or via the network. Tracking, live decoding and scheduling can all be controlled within the `Recorder`, like in legacy SatDump.

## Recorder

The `Recorder` is used to record new satellites from a software-defined radio, either directly connected to the computer or via the network. It is also possible to use the `Live Decode` feature to decode a satellite link live, without recording it first. Additionally, the `Tracking` and `Scheduling` features can be configured within the `Recorder`.

![The Recorder in use](quick_start/recorder.png)

Compared to legacy SatDump, multiple `Recorder` items can be added and operated at once.

> [!note]
>  It is not currently possible to use multiple `Recorder` items at once on a single SDR.


### Device

Choose and configure the SDR device here, including frequency, gain and other settings.

### FFT

Used to configure the FFT and waterfall parameters, and to choose the waterfall palette.

### Processing

Select a satellite pipeline and start it there, to process the satellite link live. Settings for the pipeline, such as satellite parameters or output directory, can be chosen there as well.

![Live Decode (Processing)](quick_start/live_decode.png)

### Recording

This panel allows the recording of a baseband (can be used together with the Live Decode functionality).

> [!note]
>  The output directory for recordings can be selected in the `Settings`. 

### Tracking

This panel allows the tracking of satellites or NASA Horizons objects, and gives access to scheduling, rotator control and satellite finder functionality.

### VFOs

When `Multi-mode` is configured from the `Schedule and Config` window in the `Tracking` panel, VFOs in use will appear here.

### Debug

Shows a "magic-eye" constellation to help tune the gain, especially on X-band links.

![Magic Eye patterns](quick_start/magic-eye.png)


## Processing

With the Processing window, basebands, frames or other satellite data from a pre-existing recording can be decoded. This is identical to the `Offline Processing` tab in legacy SatDump.

![Processing window](quick_start/processing.png)

Once the necessary data is input and the process is started, the full-screen window will close and a new `Processing` item will be added to the `Root`. 

![Decoding NOAA-15](quick_start/decoding.png)

While a satellite is decoding, the `Processing` window can be reopened and multiple satellites can be decoded at once.

## Products

Once decoded, satellite products are automatically added to the `Root` under the `Products` category.
There, items are organized hierarchically.

![Once NOAA-15 finished decoding, it was automatically added to the `Root`](quick_start/noaa15.png)

### Instruments

Instruments from the satellite can be selected by clicking on them.

Depending on the type of instrument, there can be different views shown on the right pane.

On imaging instruments, presets can be chosen with the `Presets` item and applied with the `Apply` button under the `Expression` item.

To save the current image, use `Current -> Save Image` in the Menu Bar.

To add overlays, such as country lines or cities, use `Current -> Add Overlay` and then click on `Apply` again.

### Projections

Projections can be added either:
* by clicking on the `Add to Proj` button on an instrument, which will add the **currently shown** composite or channel
* by adding a `Projection` with `Add -> Projection` in the Menu Bar, then selecting an instrument, selecting `Current -> Image To Handler`, and dragging the image on the Projection.

![To project this *AVHRR 221 False Color* composite, the `Add To Proj` button is used. The projection is automatically added in the `Others` section.](quick_start/proj1.png)

Clicking on the `Projection` item reveals settings for the projection. Usually, choosing `Equirectangular` and selecting both `Auto Mode` and `Auto Scale Mode` is enough to obtain a good projection.

Clicking on `Project` will project the image.

![NOAA 15 projected using the default settings](quick_start/proj2.png)

#### To project more than one instrument or satellite

To project more than one instrument or satellite: 
1. Select the new instrument (either in the same satellite or in another satellite)
2. Apply a composite or select the desired channel
3. Click on `Current -> Image to Handler`
4. Drag the new image handler over the `Projection` item
5. Click on the `Projection` item
6. Select `Project` again.

![HIRS has been selected and a composite has been applied. The image obtained after clicking `Current -> Image to Handler` can be dragged on the `Projection` item to add it to the projection.](quick_start/proj3.png)

![Completed projection](quick_start/proj4.png)

## Settings

To access the Settings, use `File -> Settings`.