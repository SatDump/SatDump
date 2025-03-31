#pragma once

/**
 * @file image.h
 */

#include <cstddef>
#include <cstdint>
#include <vector>

#if IMAGELIB_NOINLINE_SETGET
#define IMGLIB_INLINE_SETGET
#else
#define IMGLIB_INLINE_SETGET inline
#endif

namespace image
{
    class Image
    {
    protected:
        // Image buffer
        size_t data_size = 0;
        size_t type_size = 0;
        void *d_data = nullptr;

        // Size and parameters
        int d_depth = 0;
        int d_maxv = 0;
        size_t d_width = 0;
        size_t d_height = 0;
        int d_channels = 0;

    public:
        // Metadata stuff
        void *metadata_obj = nullptr; // DO NOT USE DIRECTLY!
    protected:
        void copy_meta(const Image &img);

    public:
        /**
         * @brief Init null image, with no data buffer
         */
        Image();

        /**
         * @brief Init emtpy image, set to 0 but with an allocated data buffer
         * @param bit_depth depth of the image (usually 8 or 16)
         * @param width Image width in pixels
         * @param height Image height in pixels
         * @param channels Number of channels. See init() for more info
         */
        Image(int bit_depth, size_t width, size_t height, int channels);

        /**
         * @brief Init an image, reading the contents of the data buffer from
         * a provided buffer. This is useful to for example turn a uint16_t buffer
         * directly into an usable image.
         * @param buffer pointer to read from
         * @param bit_depth depth of the image (usually 8 or 16)
         * @param width Image width in pixels
         * @param height Image height in pixels
         * @param channels Number of channels. See init() for more info
         */
        Image(void *buffer, int bit_depth, size_t width, size_t height, int channels);

        Image(const Image &img); // Copy constructor
        ~Image();                // Destructor

        Image &operator=(const Image &img);

    public:
        /**
         * @brief Convert this image from B&W to RGB (if it is B&W / RGBA)
         */
        void to_rgb();

        /**
         * @brief Convert this image from to RGBA (if it is B&W / RGB)
         */
        void to_rgba();

        /**
         * @brief Convert to 8-bits. Returns the current image if it's already 8-bits
         */
        Image to8bits();

        /**
         * @brief Convert to 16-bits. Returns the current image if it's already 16-bits
         */
        Image to16bits();

        /**
         * @brief Converts to a specific target bit depth
         * @param bit_depth to convert to
         * @return converted image
         */
        Image to_depth(int bit_depth);

    public:
        /**
         * @brief Draw a B&W Image onto an image channel, or if channel
         * is kept at 0, RGB or RGBA images as well.
         * @param channel index. If 0, draws as color image
         * @param image the image to draw
         * @param x offset
         * @param y offset
         */
        void draw_image(int channel, Image image, int x = 0, int y = 0);

        /**
         * @brief Draw an alpha Image onto an image or if channel
         * is kept at 0, RGB or RGBA images as well.
         * Respects the image's Alpha channel.
         * @param channel index. If 0, draws as color image
         * @param image the image to draw
         * @param x offset
         * @param y offset
         */
        void draw_image_alpha(Image image, int x = 0, int y = 0);

        /**
         * @brief Set a pixel's color
         * @param x position
         * @param y position
         * @param color of the pixel
         */
        void draw_pixel(size_t x, size_t y, std::vector<double> color);

        /**
         * @brief Draw a line with Bresenham's algorithm
         * @param x0 line start X position
         * @param y0 line start Y position
         * @param x1 line stop X position
         * @param y1 line stop Y position
         * @param color of the line
         */
        void draw_line(int x0, int y0, int x1, int y1, std::vector<double> color);

        /**
         * @brief Draw a circle with Bresenham's Midpoint algorithm
         * @param x0 center X position
         * @param y0 center Y position
         * @param radius in pixels
         * @param color of the circle
         * @param fill should be true if the circle is to be filled
         */
        void draw_circle(int x0, int y0, int radius, std::vector<double> color, bool fill = false);

        /**
         * @brief Draw a rectangle onto the image
         * @param x0 rectangle start X position
         * @param y0 rectangle start Y position
         * @param x1 rectangle stop X position
         * @param y1 rectangle stop Y position
         * @param color of the rectangle
         * @param fill should be true if the rectangle is to be filled
         */
        void draw_rectangle(int x0, int y0, int x1, int y1, std::vector<double> color, bool fill = true);

    public:
        /**
         * @brief Init (empty, filled with zeroes) image buffer
         * @param bit_depth Image bit depth. Usually should be 8 or 16
         * @param width Image width in pixels
         * @param height Image height in pixels
         * @param channels Number of channels (1 = Grayscale, 2 = GA, 3 = RGB, 4 = RGBA)
         */
        void init(int bit_depth, size_t width, size_t height, int channels);

        /**
         * @brief Delete current buffer (and set size to 0)
         */
        void clear();

    public:
        /**
         * @brief Clamp input value to what this image can handle
         * @param input the input value
         * @return clamped value (depends on bit depth)
         */
        int clamp(int input);

        /**
         * @brief Clamp input value to what this image can handle as float (0, 1)
         * @param input the input value
         * @return value between 0 and 1
         */
        double clampf(double input);

        /**
         * @brief Returns the RAW void pointer. MUST ONLY BE USED IF YOU KNOW WHAT YOU ARE DOING!
         * @return the void pointer. Be. Careful.
         */
        void *raw_data() const { return d_data; }

        /**
         * @brief Returns image bit depth
         * @return bit depth
         */
        int depth() const { return d_depth; }

        /**
         * @brief Returns image type size (in bytes)
         * @return type size, bytes
         */
        int typesize() const { return type_size; }

        /**
         * @brief Returns image width
         * @return image width in pixels
         */
        size_t width() const { return d_width; }

        /**
         * @brief Returns image height
         * @return image height in pixels
         */
        size_t height() const { return d_height; }

        /**
         * @brief Returns image channel count
         * @return number of image channels
         */
        int channels() const { return d_channels; }

        /**
         * @brief Returns image data size
         * @return Image data size, in pixels
         */
        size_t size() const { return data_size; }

        /**
         * @brief Returns image max value
         * @return maximum int value of an image pixels
         */
        int maxval() const { return d_maxv; }

    public:
        /**
         * @brief Crop an image region. Must be with x0 <= x1 and y0 <= y1
         * @param x0 x position of the first pixel
         * @param y0 y position of the first pixel
         * @param x1 x position of the second pixel
         * @param y1 y position of the second pixel
         */
        void crop(int x0, int y0, int x1, int y1);

        /**
         * @brief Crop an image region, x axis only
         * @param x0 x position of the first column
         * @param x1 x position of the second column
         */
        void crop(int x0, int x1);

        /**
         * @brief Crop an image region. Must be with x0 <= x1 and y0 <= y1.
         * Provides image as a return value
         * @param x0 x position of the first pixel
         * @param y0 y position of the first pixel
         * @param x1 x position of the second pixel
         * @param y1 y position of the second pixel
         * @return Cropped image
         */
        Image crop_to(int x0, int y0, int x1, int y1);

        /**
         * @brief Crop an image region, x asus only
         * Provides image as a return value
         * @param x0 x position of the first column
         * @param x1 x position of the second column
         * @return Cropped image
         */
        Image crop_to(int x0, int x1);

    public:
        /**
         * @brief Mirror the image
         * @param x mirror on X axis
         * @param y mirror on Y axis
         */
        void mirror(bool x, bool y);

    public:
        /**
         * @brief Resize image, using a simple pixel scaling attribution
         * (not the best, but fast)
         * @param width x size to resize to
         * @param height y size to resize to
         */
        void resize(int width, int height);

        /**
         * @brief Resize image, to another image. Same as resize otherwise
         * @param width x size to resize to
         * @param height y size to resize to
         * @return output image
         */
        Image resize_to(int width, int height);

        /**
         * @brief Resize image, using a bilinear algorithm
         * @param width x size to resize to
         * @param height y size to resize to
         * @param text_mode makes any pixel that is no 0 the maximum value of the pixel
         */
        void resize_bilinear(int width, int height, bool text_mode = false);

        /**
         * @brief Get a pixel from the image, but with the option to interpolate
         * fractional pixels.
         * @param channel index of the channel to use
         * @param x position
         * @param y position
         */
        int get_pixel_bilinear(int channel, double x, double y);

    public:
        /**
         * @brief Fill image with a single value
         * @param val the value
         */
        void fill(int val);

        /**
         * @brief Fill image with a color
         * @param color the color, must match the number of image channels
         */
        void fill_color(std::vector<double> color);

    public:
        /**
         * @brief Standard int set. No bound check!
         * Variants to pass a channel index and X / Y position
         * are also available.
         * @param p index of the pixel to set
         * @param v value to set
         */
        IMGLIB_INLINE_SETGET void set(size_t p, int v)
        {
            if (d_depth > 8)
                ((uint16_t *)d_data)[p] = v;
            else
                ((uint8_t *)d_data)[p] = v;
        }

        /**
         * @brief Standard int get. No bound check!
         * Variants to pass a channel index and X / Y position
         * are also available.
         * @param p index of the pixel to get
         * @return pixel value
         */
        IMGLIB_INLINE_SETGET int get(size_t p)
        {
            if (d_depth > 8)
                return ((uint16_t *)d_data)[p];
            else
                return ((uint8_t *)d_data)[p];
        }

        /**
         * @brief Standard float set. No bound check!
         * Variants to pass a channel index and X / Y position
         * are also available.
         * @param p index of the pixel to set
         * @param v value to set between 0 and 1
         */
        IMGLIB_INLINE_SETGET void setf(size_t p, double v)
        {
            if (d_depth > 8)
                ((uint16_t *)d_data)[p] = v * (double)d_maxv;
            else
                ((uint8_t *)d_data)[p] = v * (double)d_maxv;
        }

        /**
         * @brief Standard float get. No bound check!
         * Variants to pass a channel index and X / Y position
         * are also available.
         * @param p index of the pixel to get
         * @return pixel value between 0 and 1
         */
        IMGLIB_INLINE_SETGET double getf(size_t p)
        {
            if (d_depth > 8)
                return (double)((uint16_t *)d_data)[p] / (double)d_maxv;
            else
                return (double)((uint8_t *)d_data)[p] / (double)d_maxv;
        }

        // Easier int set/get
        IMGLIB_INLINE_SETGET void set(size_t channel, size_t p, int v) { set(channel * d_width * d_height + p, v); }
        IMGLIB_INLINE_SETGET int get(size_t channel, size_t p) { return get(channel * d_width * d_height + p); }
        IMGLIB_INLINE_SETGET void set(size_t channel, size_t x, size_t y, int v) { set(channel * d_width * d_height + y * d_width + x, v); }
        IMGLIB_INLINE_SETGET int get(size_t channel, size_t x, size_t y) { return get(channel * d_width * d_height + y * d_width + x); }

        // Easier double setf/getf
        IMGLIB_INLINE_SETGET void setf(size_t channel, size_t p, double v) { setf(channel * d_width * d_height + p, v); }
        IMGLIB_INLINE_SETGET double getf(size_t channel, size_t p) { return getf(channel * d_width * d_height + p); }
        IMGLIB_INLINE_SETGET void setf(size_t channel, size_t x, size_t y, double v) { setf(channel * d_width * d_height + y * d_width + x, v); }
        IMGLIB_INLINE_SETGET double getf(size_t channel, size_t x, size_t y) { return getf(channel * d_width * d_height + y * d_width + x); }
    };

    /**
     * @brief Copy part of an image over. Safe way to do it with variable bit
     * depths & channels
     * @param img1 image to copy to
     * @param pos1 position to copy to
     * @param img2 image to copy from
     * @param pos2 position to copy from
     * @param px_size number of pixels to copy
     */
    void imemcpy(Image &img1, size_t pos1, Image &img2, size_t pos2, size_t px_size);

    /**
     * @brief Convert an image to a 8-bit RGBA buffer for OpenGL/etc textures
     * @param img image to convert
     * @param output output buffer
     */
    void image_to_rgba(Image &img, uint32_t *output);
}