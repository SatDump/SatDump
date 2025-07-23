# LUT Generator

The LUT Generator is a tool intended to help create and edit monodimensional (1D) color look-up tables (LUTs). It is designed to be functionally similar to the *Color Tables* tool in McIDAS.

As with all tools in SatDump, more than one LUT Generator can be opened and operated at once.

## General Overview

![Overview screen](lut_generator/general.png)

The LUT Generator is composed of four main parts:
1. LUT plot: this part represents the currently edited LUT and can be interacted with using the mouse.
2. LUT controls: this part controls the LUT size, the editing mode, the color, ranges and other features related to the LUT.
3. Preview image controls: allows loading an image to test the LUT being edited in real time.
4. Preview image: shows the previously loaded image with the current LUT applied to it.

The `Current` menu in the top menu bar serves to save and load a LUT. Only the `PNG` format is supported.

## LUT controls

* `Size`: controls the size of the LUT. Clicking on `+` or `-` will increase or decrease the LUT size by one cell (pixel) at once, while clicking on `++` or `--` will do so by 10 cells at once.
* `Mode`: controls the editing mode.
    * `Fill`: clicking the mouse on the `LUT plot` will fill that cell with the currently specified color. Dragging the mouse on the `LUT Plot` will fill all the cells with the currently specified color.
    In this example, cells 50, 70 and from 90 to 100 were filled with red.
    ![Fill 50, 70 and from 90 to 100](lut_generator/fill.png)
    * `Interpolate`: clicking and dragging the mouse from one cell to another with a different color will automatically generate an interpolated list of colors between the color of the cell where the mouse was clicked and the color of the cell where the mouse was released.
    In this example, first the cell numbered 50 was filled with red, then the cell numbered 0 was interpolated with the cell numbered 50 and the cell number 50 was interpolated with the cell numbered 100.
        * ![Fill 50](lut_generator/interpolate_a.png)
        * ![Interpolate from 0 to 50 and from 50 to 100](lut_generator/interpolate_b.png)
* `Color`: controls the current color. By clicking on the box a new color can be chosen or inputted using RGBA, HSVA or HTML-style notations.
* `Pick`: leverages `grabc` to pick colors from any location on the computer screen, such as a Web image or another program.
    > [!warning]
    > This function is not currently supported on Windows and MacOS platforms.
* `Undo`: reverts the last change.
* `Invert`: inverts the LUT
* `Use Range`, `Range Min` and `Range Max`: enables the range feature to more easily compare the LUT to an actual physical quantity such as temperature, and provides features to control such range. The scale on the `LUT plot` is automatically updated to match the chosen range.

* `Replicate`: replicates the current LUT by the chosen number of times. In this example, the default LUT was replicated six times.
    ![Replicate by six times](lut_generator/replicate_6.png)
* `Interpolate`: interpolates the current LUT to the specified number of cells. In this example, the default LUT was interpolated to six cells.
    ![Interpolate by six cells](lut_generator/interpolate_6.png)

 ## Preview Image controls

* `Open`: loads a specified image. All image types supported by SatDump are allowed.
* `Refresh Preview`: refreshes the preview.

Similarly to the other image viewers in SatDump, the preview image can be dragged with the left mouse button, can be zoomed in or out with the mouse scrollwheel and the zoom can be set to fit the preview size by double-clicking the right mouse button.

![A 32 cell LUT applied to an AVHRR channel 4 image](lut_generator/avhrr_4.png)

## Current menu

* `Save LUT`: saves the current LUT to a PNG file.
* `Load LUT`: replaces the current LUT with one from a PNG file. 

## Tips

* It is possible to save a satellite channel image as calibrated and set the `Range Min` and `Range Max` to equal the ranges chosen in the calibration process.
* It is also possible to easily replicate LUTs found in papers or used by meteorological agencies by using the `Pick` function and the `Range` function. Normally only a limited number of colors need to be picked and most can be obtained with the `Interpolate` editing mode.