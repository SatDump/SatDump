#pragma once

/**
 * @file text.h
 * @brief Image Text drawing
 */

#include "image.h"
#include "image/font/imstb_truetype.h"
#include <string>

namespace satdump
{
    namespace image
    {
        /**
         * @brief Text image drawer, used to draw
         * arbitrary text onto an image. This needs to be
         * its own class as the font needs to be loaded and
         * kept around as a pointer.
         */
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

            /**
             * @brief Init the font
             * @param font_path path to a .ttf file
             */
            void init_font(std::string font_path);

            /**
             * @brief Init with the default font
             */
            void init_font();

            /**
             * @brief Is the font ready?
             * @return true if the font is ready
             */
            bool font_ready() { return has_font; }

            /**
             * @brief Draw text onto an image
             * @param img to draw onto
             * @param xs0 start X position
             * @param ys0 start Y position
             * @param color of the text
             * @param size size/height (in pixels) of the text to draw
             * @param text string the draw
             */
            void draw_text(Image &img, int xs0, int ys0, std::vector<double> color, int size, std::string text);
        };
    } // namespace image
} // namespace satdump