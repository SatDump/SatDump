#pragma once

#include "image.h"
#include "common/image/font/imstb_truetype.h"
#include <string>

namespace image
{
    class TextDrawer
    {
    protected:
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

    protected:
        // font rendering stuff
        font_info font;
        bool has_font = false;
        uint8_t *ttf_buffer;

    public:
        ~TextDrawer();
        void init_font(std::string font_path);                                                               // Init the font
        bool font_ready() { return has_font; }                                                               // Is the font ready?
        void draw_text(Image &img, int xs0, int ys0, std::vector<double> color, int size, std::string text); // Draw text onto the image
    };
}