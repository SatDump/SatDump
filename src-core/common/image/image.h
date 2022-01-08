#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

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
        int d_width = 0;
        int d_height = 0;
        int d_channels = 0;

    public:
        // Init
        void init(int width, int height, int channels); // Init image buffer
        void clear();                                   // Delete current buffer. To clear the image with a color instead, use fill()

        // Operators
        T &operator[](int i) { return d_data[i]; }
        Image<T> &operator=(const Image<T> &img);
        Image<T> &operator<<=(const int &shift);

        // Utils
        T clamp(int input); // Clamp input value to what this image can handle

        int depth() const { return d_depth; }       // Returns image bit depth
        int width() const { return d_width; }       // Returns image width
        int height() const { return d_height; }     // Returns image height
        int channels() const { return d_channels; } // Returns image channel count
        int size() const { return data_size; }      // Returns image data size

        T *data() { return d_data; }                                              // Return the raw image data buffer
        T *channel(int channel) { return &d_data[d_width * d_height * channel]; } // Return a pointer to a specific channel

        void to_rgb();              // Convert this image from B&W to RGB (if it is B&W)
        Image<uint8_t> to8bits();   // Convert to 8-bits. Returns the current image if it's already 8-bits
        Image<uint16_t> to16bits(); // Convert to 16-bits. Returns the current image if it's already 16-bits

        // Image processing functions
        void fill_color(T color[]);                                          // Fill image with a single color
        void fill(T val);                                                    // Fill image with a single value
        void mirror(bool x, bool y);                                         // Mirror the image along the X or Y axis
        Image<T> &equalize();                                                // Perform histogram equalization
        Image<T> &normalize();                                               // Normalize the current image data. This takes the current min and max values and extends it to the full bit depth range
        void white_balance(float percentileValue = 0.05f);                   // While balance algorithm from Gimp
        void crop(int x0, int y0, int x1, int y1);                           // Crop an image region. Must be with x0 <= x1 and y0 <= y1
        void crop(int x0, int x1);                                           // Crop an image region, x axis only
        void resize(int width, int height);                                  // Resize image, using a simple pixel scaling attribution (not the best, but fast)
        void resize_bilinear(int width, int height, bool text_mode = false); // Resize image, using a bilinear algorithm
        void brightness_contrast_old(float brightness, float contrast);      // Brightness-Contrast algorithm from old Gimp versions
        void linear_invert();                                                // Invert the entire image
        void simple_despeckle(int thresold = 10);                            // Very basic despeckle algorithm

        // Drawing functions
        void draw_pixel(int x, int y, T color[]);                                                      // Set a pixel's color
        void draw_line(int x0, int y0, int x1, int y1, T color[]);                                     // Draw a line with Bresenham's algorithm
        void draw_circle(int x0, int y0, int radius, T color[], bool fill = false);                    // Draw a circle with Bresenham's Midpoint algorithm
        void draw_image(int channel, Image<T> image, int x = 0, int y = 0);                            // Draw a B&W Image onto a channel
        void draw_text(int x0, int y0, T color[], std::vector<Image<uint8_t>> font, std::string text); // Draw text onto the image

    public:
        Image();                                               // Init null image
        Image(int width, int height, int channels);            // Init emtpy image, set to 0
        Image(T *buffer, int width, int height, int channels); // Init from existing buffer
        Image(const Image &img);                               // Copy constructor
        ~Image();                                              // Destructor

    public:
        // PNG Interface
        void save_png(std::string file, bool fast = true); // Save to a PNG file. Defaults to fast-saving with no filters
        void load_png(std::string file);                   // Load a PNG file

        // JPEG Interface
        void load_jpeg(std::string file); // Load a JPEG file
    };

    // Others

    // Font creation
    std::vector<Image<uint8_t>> make_font(int size); // Generate a font to be used by draw_text. Uses the bundled GNU FreeMono

    // LUT functions
    template <typename T>
    Image<T> create_lut(int channels, int width, int points, std::vector<T> data);
    template <typename T>
    Image<T> scale_lut(int width, int x0, int x1, Image<T> in);

    // LUT presets
    template <typename T>
    Image<T> LUT_jet();
}