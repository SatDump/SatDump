# Image Product Expression

The image product expression system is specifically used to generate (more or less complex) RGB composites from an ImageProduct.

## Expression

An expression is split into 2 parts, one of them optional. The first part is the actual pixel expression, where you can express the desired output value for each channel via a mathematical expression based on the raw channel values. Possible combinations are summarized in this table  :

| Expression         | Output          |
|--------------------|-----------------|
| ch1                | Grayscale       |
| ch1, ch2           | Grayscale Alpha |
| ch1, ch2, ch3      | RGB             |
| ch1, ch2, ch3, ch4 | RGB Alpha       |

![AVHRR ch2, ch2, ch1 Expression Example](img_exp/simple.png)

The only variables available by default are raw channel values, named `chCHNAME`, with `CHNAME` being the instrument channel name, as displayed in the explorer under "Channels" (eg, `Channel 3a` becomes `ch3a`).

Those channel variables will be scaled from 0 to 1, with 0 being the minimum possible pixel value, and 1 the maximum. The expected output range is the same.

## Additional definitions

In many situations it will be desirable to utilize more than just raw instrument channels. To this effect, it is possible to define many other types of variables.  

Those definitions must be present in the expression because pixel values, and separated by `;` instead of a normal coma. For example :

```
cch1=(1, reflective_radiance, 0.000000, 46.203000);
cch2=(1, reflective_radiance, 0.000000, 46.203000);
ch2, ch2, ch1
```

### Calibrated Channel

This allows getting calibrated channels values as a variable.

![Calibration Example](img_exp/cal.png)

```
variable_name=(CHNAME, CALIBRATION_UNIT, MINCALVAL, MAXCALVAL);
```

- `variable_name` should be the desired variable name. Often `cchCHNAME`
- `CALIBRATION_UNIT` should be calibration unit ID string (TODOREWORK document)
- `MINCALVAL` value of the unit to be used as a minimum (scaled to 0)
- `MAXCALVAL` value of the unit to be used as a maximum (scaled to 1)

If `CALIBRATION_UNIT` is set to `equalized`, this will return the raw channel equalized (useful to stretch it on uncalibrated instruments for example).

### Look-up table (1D/2D)

This lets you loading and utilizing a Look-Up Table in the expression.

![LUT Example](img_exp/lut.png)

```
lut_name=lut(path/to/lut.png)
```

- `lut_name` desired LUT function name
- `path/to/lut.png` path to a lut (can be any image format) relative to SatDump's resources folder

The lut can then be used as follow in an expression :

```
lut_name(x_pos, y_pos, lut_channel)
```

- `x_pos` LUT X position, expects 0 to 1
- `y_pos` LUT Y position, expects 0 to 1
- `lut_channel` LUT Channel index to get. Can be 0 up to the number of channels in the LUT image minus 1

Example to generate RGB with a LUT from a single channel :

```
lut=lut(path/to/lut.png);
lut(ch1, 0, 0), lut(ch1, 0, 1), lut(ch1, 0, 2)
```

### Equirectangular Projected

Loads an equirectangular image (such as a NASA Blue Marble) and (given the instrument has projection information) will return its pixel value so it can be used just like any other channel.

![EquP Example](img_exp/equ.png)

```
equ_name=equp(path/to/image.png)
```

- `equ_name` desired EquP function name
- `path/to/image.png` Equirectangular image to load, relative to SatDump's resources folder

```
equ_name(equ_channel)
```

- `equ_channel` Equirectangular Channel index to get. Can be 0 up to the number of channels in the equirectangular image minus 1

Example to generate a RGB "MCIR" composite :

```
bm=equp(maps/nasa_hd.jpg); 
ch4 * (ch4) + (1-ch4) * bm(0), 
ch4 * (ch4) + (1-ch4) * bm(1), 
ch4 * (ch4) + (1-ch4) * bm(2)
```

### Macros

Since some equations can become relatively complex, this allows having a simple concept of macros to simplify them. **Do note that macros cannot be nested!**

![Alignement Example, made obvious by some missing lines in AWS MWR Data](img_exp/macro.png)

```
macro_name = equation
```

## Channel alignement/scaling

For several instruments, channels are not actually aligned with one or the other, or may be of different resolutions. SatDump will automatically deal with this.

![Macro Example](img_exp/align.png)

The output alignment and size will be dictated by the first channel found in the equation. Therefore `ch2, ch2, ch1` will lead to `ch2` being used as the output size/offset, while `ch1 * 0 + ch2, ch2, ch1` will generate the same output but using `ch1` as an output reference. This behaviour can be overwritten by explicitely specifying `ref_channel=1` if necessary. Do note said channel *must* be part of the expression still.

## Wavelength/Frequency-based Channel selection

A lot of composites are common between several instruments, and re-writing them for each can be cumbersome. Instead, it is possible to automatically select a channel by specifying a wavelength or frequency.

Using `{10.4um}` will select the channel closest to 10.4um in the loaded instrument, and effectively will be getting replaced by `4` or whatever the actual channel number is. If you had `ch{10.4um}`, this would become `ch4`.

This works the same with calibrated channels :
```
cch2=({830nm}, sun_angle_compensated_reflective_radiance, 0.000000, 97.050700);
cch1=({630nm}, sun_angle_compensated_reflective_radiance, 0.000000, 90.000000);
cch2, cch2, cch1
```