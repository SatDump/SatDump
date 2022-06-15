# Products Specification

The products format in SatDump shall be the output of an instrument decoding step. The idea is not to make a format like HDF-5 entirely taking over storing the actual data, but more to store what can't be stored in let's say, an image file or other "user-friendly" formats.

Before 1.0, it was considered to move over SatDump's outputs to HDF-5 or similar formats, but that was abandonned as in most cases, all that's really required is storing some image data or readings, something... PNG for example will cover perfectly without requiring any usual SW for users to work with them.
For things like radiation that cannot be stored in any really standard format, there before was no way to store the raw readings either.

Hence, such a products system should be both able to store metadata (projection parameters for example, replacing the previous .georef file format) and actual data if no other format can be used to better fit the purpose.

The products file uses a JSON-like format called CBOR, which being binary-based allows for smaller filesizes. It should be written in the instrument's output directory alongside other files such as images.

Another main goal of this format is to generalize processing as much as possible. Any imager for example should be able to utilize this system with minor changes and pre-processing, instead of having custom code for each.

## Common Fields

The fields enumerated here should be common to ANY products file.

- `instrument` : Instrument name, eg `avhrr`, `modis`, `sem`. This is the unique instrument ID (lowercase) allowing to identify what satellite instrument this data is from, and hence let further processing SW decide what to do. If there are several massively different versions of a same instrument, this ID should be different.
- `type` : Products type. This holds an unique string ID (lowercase) identifying the product sub-type. 
- `tle` : Optional field holding a TLE struct. This should be the TLEs revelant for processing of the provided data, given it is necessary.


## Image Products

The type of those products should be set to `image`. This products type aims at storing anything that can be treated as image data. This should cover all types of satellite imagers and sounders that do cover more than a single point (and hence, do image a decent swath).

After all, further processing of a Visible & IR spectrometer will usually be either about composing several channels to get a color image or converting to radiance and perhaps temperatures to then apply a LUT or feed to some other algorithm. That's pretty much the same as you would do for a microwave sounder, and not far off what could be done with an infrared sounder capable of covering several layers of the atmosphere. Therefore, many common processing and metadata needed for processing are the same for all those instruments, allowing to cover so many in a pretty generic format.

Image data for those products should be stored in .png files of either 16 or 8-bit depth.

- `bit_depth` : Actual bit depth of the image data
- `needs_correlation` : This should be set to true if there is no guarantee all channels are synced between each-other. This is the case on LRPT and HRD for example.
- `save_as_matrix` : Some sounders, such as IASI can have several thousands channels. In those cases, it may be preferred to save them all in a single .png. Setting this option to true will do so.
- `images` : Actual image channels, which also have several sub-fields.
    - `file` : Path to the .png file to load, relative
    - `name` : Channel name. Usually a single number, but can be "m5" or similar where convenient to stay in line with official channel naming.
    - `ifov_y` : IFOV size vertically. Only required if needs_correlation is true and timestamps don't simply cover a scanline. 
    - `ifov_x` : IFOV size horizontally. Same as above.
    - `offset_x` : Some instruments have channels offset from each other by a bit. This allows accounting for it.
- `wavenumbers` : Vector of wavenumbers for each channel. Optional, but may be required for calibration


- `has_timestamps` : Boolean indicating the presence or lack of timestamps alongside the image.
- `timestamps_type` : TBD, maybe switch to a string? Optional
- `timestamps` : The actual timestamps data. If it varies between images, it should be set per-image. Optional

- `projection_cfg` : Projection configuration for geo-referencing the imagery data. Optional

- `calibration` : Calibration settings to convert the raw imagery data to physical units. Optional. Rest TBD
