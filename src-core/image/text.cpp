#include "text.h"
#include "core/resources.h"

#define STB_TRUETYPE_IMPLEMENTATION // force following include to generate implementation
#include "image/font/imstb_truetype.h"

#ifndef _MSC_VER
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#include "font/utf8.h"

#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif
#include <fstream>
#include <iostream>

#include "logger.h"

namespace satdump
{
    namespace image
    {
        TextDrawer::~TextDrawer()
        {
            if (has_font)
            {
                for (auto &v : font.chars)
                    free(v.bitmap);
                font.chars.clear();
                delete[] ttf_buffer;
            }
        }

        void TextDrawer::init_font(std::string font_path)
        {
            std::ifstream infile(font_path, std::ios::binary);

            if (!infile.good())
                return;
            // get length of file
            infile.seekg(0, std::ios::end);
            size_t length = infile.tellg();
            infile.seekg(0, std::ios::beg);
            ttf_buffer = new uint8_t[length];
            // read file
            infile.read((char *)ttf_buffer, length);

            stbtt_fontinfo fontp;
            stbtt_InitFont(&fontp, ttf_buffer, stbtt_GetFontOffsetForIndex(ttf_buffer, 0));

            stbtt_GetFontBoundingBox(&fontp, &font.x0, &font.y0, &font.x1, &font.y1);
            stbtt_GetFontVMetrics(&fontp, &font.asc, &font.dsc, &font.lg);

            font.fontp = fontp;
            infile.close();
            has_font = true;
        }

        void TextDrawer::init_font() { init_font(resources::getResourcePath("fonts/font.ttf")); }

        void TextDrawer::draw_text(Image &img, int xs0, int ys0, std::vector<double> color, int s, std::string text)
        {
            if (!has_font)
            {
                logger->error("Tried to draw text without having a font initialized!");
                return;
            }
            int CP = 0;
            std::vector<char> cstr(text.c_str(), text.c_str() + text.size() + 1);
            char *c = cstr.data();
            float SF = stbtt_ScaleForPixelHeight(&font.fontp, s);
            int BL = SF * font.y1;

            char_el info;

            while (c < c + cstr.size())
            {
                try
                {
                    info.char_nb = utf8::next(c, c + cstr.size());
                }
                catch (utf8::invalid_utf8 &)
                {
                    break;
                }

                if (info.char_nb == '\0')
                    break;

                if (info.char_nb == 0xA)
                { // EOL
                    ys0 += SF * (font.asc - font.dsc + font.lg);
                    CP = 0;
                    continue;
                }

                bool f = false;
                for (unsigned int k = 0; k < font.chars.size(); k++)
                    if (font.chars[k].char_nb == info.char_nb)
                    {
                        if (font.chars[k].size != s)
                        {
                            font.chars.erase(font.chars.begin() + k);
                            break;
                        }
                        f = true;
                        info = font.chars[k];
                        break;
                    }

                if (!f)
                {
                    // bitmap = stbtt_GetGlyphBitmap(&font.fontp, 0, SF, info.char_nb, &info.w, &info.h, 0, 0);
                    info.glyph_nb = stbtt_FindGlyphIndex(&font.fontp, info.char_nb);
                    stbtt_GetGlyphBox(&font.fontp, info.glyph_nb, &info.cx0, &info.cy0, &info.cx1, &info.cy1);
                    stbtt_GetGlyphBitmapBox(&font.fontp, info.glyph_nb, SF, SF, &info.ix0, &info.iy0, &info.ix1, &info.iy1);
                    stbtt_GetGlyphHMetrics(&font.fontp, info.glyph_nb, &info.advance, &info.lsb);
                    info.w = abs(info.ix1 - info.ix0);
                    info.h = abs(info.iy1 - info.iy0);
                    info.bitmap = (unsigned char *)malloc(info.w * info.h);
                    info.size = s;
                    memset(info.bitmap, 0, info.w * info.h);
                    stbtt_MakeGlyphBitmap(&font.fontp, info.bitmap, info.w, info.h, info.w, SF, SF, info.glyph_nb);
                    font.chars.push_back(info);
                }

                int pos = 0;
                for (int j = 0; j < info.h; ++j)
                    for (int i = 0; i < info.w; ++i)
                    {
                        unsigned char m = info.bitmap[pos];
                        int x = xs0 + i + CP + SF * info.lsb, y = j + BL - info.cy1 * SF + ys0;
                        unsigned int pos2 = y * img.width() + x;
                        if (m != 0 && pos2 < img.width() * img.height())
                        {
                            float mf = m / 255.0;
                            std::vector<double> col = {(color[0] - img.getf(0, pos2)) * mf + img.getf(0, pos2), img.channels() > 1 ? ((color[1] - img.getf(1, pos2)) * mf + img.getf(1, pos2)) : 1,
                                                       img.channels() > 2 ? ((color[2] - img.getf(2, pos2)) * mf + img.getf(2, pos2)) : 1,
                                                       img.channels() > 3 ? ((color[3] - img.getf(3, pos2)) * mf + img.getf(3, pos2)) : 1};
                            if (x < (int)img.width())
                                img.draw_pixel(x, y, col);
                        }
                        pos++;
                    }
                // if (!f)
                // stbtt_FreeBitmap(bitmap, font.fontp.userdata);

                CP += SF * info.advance;
            }
        }
    } // namespace image
} // namespace satdump