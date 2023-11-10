#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <functional>
#include <utility>

#include "common/image/font/imstb_truetype.h"

struct char_el
{
    int char_nb;
    int glyph_nb;
    unsigned char *bitmap;
    int cx0, cx1, cy0, cy1, advance, lsb, w, h, ix0, ix1, iy0, iy1, size;
};

struct font_info
{
    struct stbtt_fontinfo fontp;
    std::string path;
    int x0, x1, y0, y1, asc, dsc, lg, wdh, hht;
    std::vector<char_el> chars;
};

namespace image
{
    template <typename T>
    class Image
    {
    private:
        // Image buffer
        bool has_data = false;
        size_t data_size = 0;
        T *d_data;

        // Size and parameters
        int d_depth = 0;
        size_t d_width = 0;
        size_t d_height = 0;
        int d_channels = 0;

        // font rendering stuff
        font_info font;
        bool has_font = false;
        uint8_t *ttf_buffer;

    public:
        // Init
        void init(size_t width, size_t height, int channels); // Init image buffer
        void clear();                                         // Delete current buffer. To clear the image with a color instead, use fill()
        void init_font(std::string font_path);

        // Operators
        T &operator[](size_t i) { return d_data[i]; }
        Image<T> &operator=(const Image<T> &img);
        Image<T> &operator<<=(const int &shift);

        // Utils
        T clamp(int input); // Clamp input value to what this image can handle

        int depth() const { return d_depth; }       // Returns image bit depth
        size_t width() const { return d_width; }    // Returns image width
        size_t height() const { return d_height; }  // Returns image height
        int channels() const { return d_channels; } // Returns image channel count
        size_t size() const { return data_size; }   // Returns image data size

        T *data() { return d_data; }                                              // Return the raw image data buffer
        T *channel(int channel) { return &d_data[d_width * d_height * channel]; } // Return a pointer to a specific channel
        T wraparound_read(T* c, int x, int y);                                    // Returns pixel value, wrapping around at the edge

        void to_rgb();              // Convert this image from B&W to RGB (if it is B&W / RGBA)
        void to_rgba();             // Convert this image from to RGBA (if it is B&W / RGB)
        Image<uint8_t> to8bits();   // Convert to 8-bits. Returns the current image if it's already 8-bits
        Image<uint16_t> to16bits(); // Convert to 16-bits. Returns the current image if it's already 16-bits

        // Info access functions
        bool font_ready() { return has_font; }

        // Image processing functions
        void fill_color(T color[]);                                          // Fill image with a single color
        void fill(T val);                                                    // Fill image with a single value
        void mirror(bool x, bool y);                                         // Mirror the image along the X or Y axis
        Image<T> &equalize(bool per_channel = false);                        // Perform histogram equalization
        Image<T> &normalize();                                               // Normalize the current image data. This takes the current min and max values and extends it to the full bit depth range
        void white_balance(float percentileValue = 0.05f);                   // While balance algorithm from Gimp
        void crop(int x0, int y0, int x1, int y1);                           // Crop an image region. Must be with x0 <= x1 and y0 <= y1
        void crop(int x0, int x1);                                           // Crop an image region, x axis only
        Image<T> crop_to(int x0, int y0, int x1, int y1);                    // Crop an image region. Must be with x0 <= x1 and y0 <= y1, returning the output
        Image<T> crop_to(int x0, int x1);                                    // Crop an image region. Must be with x0 <= x1 and y0 <= y1, returning the output
        void resize(int width, int height);                                  // Resize image, using a simple pixel scaling attribution (not the best, but fast)
        Image<T> resize_to(int width, int height);                           // Resize image, to another image
        void resize_bilinear(int width, int height, bool text_mode = false); // Resize image, using a bilinear algorithm
        void brightness_contrast_old(float brightness, float contrast);      // Brightness-Contrast algorithm from old Gimp versions
        void linear_invert();                                                // Invert the entire image
        void simple_despeckle(int thresold = 10);                            // Very basic despeckle algorithm
        void median_blur();                                                  // Median blur algorithm
        void kuwahara_filter();                                              // Adaptive noise reduction filter

        // Drawing functions
        void draw_pixel(int x, int y, T color[]);                                                      // Set a pixel's color
        void draw_line(int x0, int y0, int x1, int y1, T color[]);                                     // Draw a line with Bresenham's algorithm
        void draw_circle(int x0, int y0, int radius, T color[], bool fill = false);                    // Draw a circle with Bresenham's Midpoint algorithm
        void draw_image(int channel, Image<T> image, int x = 0, int y = 0);                            // Draw a B&W Image onto a channel
        void draw_text(int xs0, int ys0, T color[], int size, std::string text);                       // Draw text onto the image
        void draw_text(int x0, int y0, T color[], std::vector<Image<uint8_t>> font, std::string text); // old

    public:
        Image();                                                     // Init null image
        Image(size_t width, size_t height, int channels);            // Init emtpy image, set to 0
        Image(T *buffer, size_t width, size_t height, int channels); // Init from existing buffer
        Image(const Image &img);                                     // Copy constructor
        ~Image();                                                    // Destructor

    public:
        // PNG Interface
        void save_png(std::string file, bool fast = true);                      // Save to a PNG file. Defaults to fast-saving with no filters
        void load_png(std::string file, bool disableIndexing = false);          // Load a PNG file (disableIndexing only applies to indexed color images)
        void load_png(uint8_t *buffer, int size, bool disableIndexing = false); // Load a PNG from memory (disableIndexing only applies to indexed color images)

        // JPEG Interface
        void save_jpeg(std::string file);          // Save to a JPEG file
        void load_jpeg(std::string file);          // Load a JPEG file
        void load_jpeg(uint8_t *buffer, int size); // Load a JPEG from memory

        // J2K Interface
        void save_j2k(std::string file); // Save to a J2K file
        void load_j2k(std::string file); // Load a J2K file

        // PBM Interface
        void save_pbm(std::string file); // Save to a PBM file
        void load_pbm(std::string file); // Load a PBM file

        // Generic loading/saving interface
        void load_img(std::string file);                   // Load a file, auto-detecting type
        void load_img(uint8_t *buffer, int size);          // Load from memory, auto-detecting type
        void save_img(std::string file, bool fast = true); // Save file, determine type based on extension or default setting
        bool append_ext(std::string *file);                // If the filename has no extension, use the default image format
    };

    // Others

    // Font creation
    std::vector<Image<uint8_t>> make_font(int size, bool text_mode = true); // Generate a font to be used by draw_text. Uses the bundled GNU FreeMono

    template <typename T>
    Image<T> generate_text_image(std::string text, T color[], int size, int padX, int padY); // return text on a transparent background

    // LUT functions
    template <typename T>
    Image<T> create_lut(int channels, int width, int points, std::vector<T> data);
    template <typename T>
    Image<T> scale_lut(int width, int x0, int x1, Image<T> in);

    // LUT presets
    template <typename T>
    Image<T> LUT_jet();

    /*
    Simple function to make a "matrix" out of many image to save them
    in a single image.

    You should ensure img_cnt is never 0 and does not exceed what
    get_img_func can provide.
    */
    template <typename T>
    image::Image<T> make_manyimg_composite(int count_width, int count_height, int img_cnt, std::function<image::Image<T>(int cnt)> get_img_func)
    {
        image::Image<T> first_img = get_img_func(0);
        int img_w = first_img.width();
        int img_h = first_img.height();

        image::Image<uint16_t> image_all(img_w * count_width, img_h * count_height, first_img.channels());

        first_img.clear();

        for (int row = 0; row < count_height; row++)
        {
            for (int column = 0; column < count_width; column++)
            {
                if (row * count_width + column >= img_cnt)
                    return image_all;

                image_all.draw_image(0, get_img_func(row * count_width + column), img_w * column, img_h * row);
            }
        }

        return image_all;
    }
}
