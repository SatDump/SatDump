#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

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
        Image();                                                                       // Init null image
        Image(int bit_depth, size_t width, size_t height, int channels);               // Init emtpy image, set to 0
        Image(void *buffer, int bit_depth, size_t width, size_t height, int channels); // Init from existing buffer
        Image(const Image &img);                                                       // Copy constructor
        ~Image();                                                                      // Destructor

        Image &operator=(const Image &img);

    public:
        void to_rgb();                 // Convert this image from B&W to RGB (if it is B&W / RGBA)
        void to_rgba();                // Convert this image from to RGBA (if it is B&W / RGB)
        Image to8bits();               // Convert to 8-bits. Returns the current image if it's already 8-bits
        Image to16bits();              // Convert to 16-bits. Returns the current image if it's already 16-bits
        Image to_depth(int bit_depth); // Converts to a specific target bit depth

    public:
        void draw_image(int channel, Image image, int x = 0, int y = 0);                                  // Draw a B&W Image onto a channel
        void draw_pixel(size_t x, size_t y, std::vector<double> color);                                   // Set a pixel's color
        void draw_line(int x0, int y0, int x1, int y1, std::vector<double> color);                        // Draw a line with Bresenham's algorithm
        void draw_circle(int x0, int y0, int radius, std::vector<double> color, bool fill = false);       // Draw a circle with Bresenham's Midpoint algorithm
        void draw_rectangle(int x0, int y0, int x1, int y1, std::vector<double> color, bool fill = true); // Draw a rectangle onto the image

    public:
        // Init (empty) image buffer
        void init(int bit_depth, size_t width, size_t height, int channels);

        // Delete current buffer.
        void clear();

    public:
        int clamp(int input);        // Clamp input value to what this image can handle
        double clampf(double input); // Clamp input value to what this image can handle as float (0, 1)

        void *raw_data() const { return d_data; }   // Returns the RAW void pointer. MUST ONLY BE USED IF YOU KNOW WHAT YOU ARE DOING!
        int depth() const { return d_depth; }       // Returns image bit depth
        int typesize() const { return type_size; }  // Returns image type size (in bytes)
        size_t width() const { return d_width; }    // Returns image width
        size_t height() const { return d_height; }  // Returns image height
        int channels() const { return d_channels; } // Returns image channel count
        size_t size() const { return data_size; }   // Returns image data size
        int maxval() const { return d_maxv; }       // Returns image max value

    public:
        void crop(int x0, int y0, int x1, int y1);     // Crop an image region. Must be with x0 <= x1 and y0 <= y1
        void crop(int x0, int x1);                     // Crop an image region, x axis only
        Image crop_to(int x0, int y0, int x1, int y1); // Crop an image region. Must be with x0 <= x1 and y0 <= y1, returning the output
        Image crop_to(int x0, int x1);                 // Crop an image region. Must be with x0 <= x1 and y0 <= y1, returning the output

    public:
        void mirror(bool x, bool y);

    public:
        void resize(int width, int height);                                  // Resize image, using a simple pixel scaling attribution (not the best, but fast)
        Image resize_to(int width, int height);                              // Resize image, to another image
        void resize_bilinear(int width, int height, bool text_mode = false); // Resize image, using a bilinear algorithm

        int get_pixel_bilinear(int channel, double x, double y);

    public:
        void fill(int val); // Fill image with a single value

    public:
        // Standard int set. No bound check!
        inline void set(size_t p, int v)
        {
            if (d_depth > 8)
                ((uint16_t *)d_data)[p] = v;
            else
                ((uint8_t *)d_data)[p] = v;
        }

        // Standard int get. No bound check!
        inline int get(size_t p)
        {
            if (d_depth > 8)
                return ((uint16_t *)d_data)[p];
            else
                return ((uint8_t *)d_data)[p];
        }

        // Standard double set. No bound check!
        inline void setf(size_t p, double v)
        {
            if (d_depth > 8)
                ((uint16_t *)d_data)[p] = v * (double)d_maxv;
            else
                ((uint8_t *)d_data)[p] = v * (double)d_maxv;
        }

        // Standard double get. No bound check!
        inline double getf(size_t p)
        {
            if (d_depth > 8)
                return (double)((uint16_t *)d_data)[p] / (double)d_maxv;
            else
                return (double)((uint8_t *)d_data)[p] / (double)d_maxv;
        }

        // Easier int set/get
        inline void set(size_t channel, size_t p, int v) { set(channel * d_width * d_height + p, v); }
        inline int get(size_t channel, size_t p) { return get(channel * d_width * d_height + p); }
        inline void set(size_t channel, size_t x, size_t y, int v) { set(channel * d_width * d_height + y * d_width + x, v); }
        inline int get(size_t channel, size_t x, size_t y) { return get(channel * d_width * d_height + y * d_width + x); }

        // Easier double setf/getf
        inline void setf(size_t channel, size_t p, double v) { setf(channel * d_width * d_height + p, v); }
        inline double getf(size_t channel, size_t p) { return getf(channel * d_width * d_height + p); }
        inline void setf(size_t channel, size_t x, size_t y, double v) { setf(channel * d_width * d_height + y * d_width + x, v); }
        inline double getf(size_t channel, size_t x, size_t y) { return getf(channel * d_width * d_height + y * d_width + x); }
    };

    // Copy part of an image over. Safe way to do it with variable bit depths
    void imemcpy(Image &img1, size_t pos1, Image &img2, size_t pos2, size_t px_size);

    // Convert an image to a 8-bit RGBA buffer for OpenGL/etc textures
    void image_to_rgba(Image &img, uint32_t *output);
}