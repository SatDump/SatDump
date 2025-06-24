#pragma once

#include "core/resources.h"
#include "core/style.h"
#include "image/image.h"
#include "image/io.h"
#include "imgui/imgui.h"
#include "imgui/imgui_image.h"
#include "imgui/imgui_internal.h"
#include "litehtml/types.h"
#include "logger.h"
#include "utils/string.h"
#include <cstdint>
#include <litehtml.h>

class LiteHtmlView : public litehtml::document_container
{
public:
    int width;
    int height;
    litehtml::string basedir;
    //   std::map<string, Bitmap> images;

    struct FontHolder
    {
        int size;
        ImFont *font;
    };

    struct ImgHolder
    {
        int width;
        int height;
        uint32_t texId;
    };

    std::map<std::string, ImgHolder> images;

public:
    LiteHtmlView(int width, int height, litehtml::string basedir) : width(width), height(height), basedir(basedir) { logger->critical("HI"); }

    litehtml::string make_url(const char *src, const char *baseurl);

    litehtml::uint_ptr create_font(const litehtml::font_description &descr, const litehtml::document *doc, litehtml::font_metrics *fm)
    {
        logger->info("CREATE FONT 1, %d", descr.size);
        FontHolder *f = new FontHolder();

        if (descr.size > 16)
            f->font = style::bigFont;
        else
            f->font = style::baseFont;

        // fm->ascent = hgt.ascent;
        // fm->descent = hgt.descent;
        fm->height = f->font->FontSize; //(int)(hgt.ascent + hgt.descent);
                                        // fm->x_height = (int)hgt.leading;

        f->size = descr.size;

        return (litehtml::uint_ptr)f;
    }

    // litehtml::uint_ptr create_font(const char *faceName, int size, int weight, litehtml::font_style italic, unsigned int decoration, litehtml::font_metrics *fm)
    // {
    //     logger->info("CREATE FONT 2");

    //     // fm->ascent = hgt.ascent;
    //     // fm->descent = hgt.descent;
    //     fm->height = ImGui::GetDefaultFont()->FontSize; //(int)(hgt.ascent + hgt.descent);
    //     // fm->x_height = (int)hgt.leading;

    //     return (litehtml::uint_ptr)malloc(1);
    // }

    void delete_font(litehtml::uint_ptr hFont) override
    {
        // ( (FontHolder *)hFont)->font
        delete (FontHolder *)hFont;
    }

    int text_width(const char *text, litehtml::uint_ptr hFont) override
    {
        float size = ((FontHolder *)hFont)->font->FontSize;
        ((FontHolder *)hFont)->font->FontSize = ((FontHolder *)hFont)->size;
        ImGui::PushFont(((FontHolder *)hFont)->font);
        auto v = ImGui::CalcTextSize(text).x;
        ImGui::PopFont();
        ((FontHolder *)hFont)->font->FontSize = size;
        return v;
    }

    void draw_text(litehtml::uint_ptr hdc, const char *text, litehtml::uint_ptr hFont, litehtml::web_color color, const litehtml::position &pos) override
    {
        float size = ((FontHolder *)hFont)->font->FontSize;
        ((FontHolder *)hFont)->font->FontSize = ((FontHolder *)hFont)->size;
        ImGui::PushFont(((FontHolder *)hFont)->font);
        // logger->error("ADTODOTEXT %d %d , %s\n", pos.x, pos.y, text);
        auto p = ImGui::GetWindowPos();
        ImGui::GetWindowDrawList()->AddText({p.x + (float)pos.x, p.y + (float)pos.y}, ImColor(color.red, color.green, color.blue, color.alpha), text);
        ImGui::PopFont();
        ((FontHolder *)hFont)->font->FontSize = size;
    }

    int pt_to_px(int pt) const override { return pt; }

    int get_default_font_size() const override { return ImGui::GetDefaultFont()->FontSize; }

    const char *get_default_font_name() const override { return "default"; }

    void load_image(const char *src, const char *baseurl, bool redraw_on_ready) override
    {
        std::string path;
        if (baseurl != nullptr)
            path += baseurl;
        if (src != nullptr)
            path += src;

        logger->error("TODOIMGLOAD");
        logger->info("IMG " + path);

        if (!images.count(path))
        {
            satdump::image::Image im;
            satdump::image::load_img(im, "documentation/html/" + path);
            if (im.size())
            {
                ImgHolder h;
                h.width = im.width();
                h.height = im.height();
                h.texId = makeImageTexture();

                uint32_t *output_buffer = new uint32_t[im.width() * im.height()];
                image::image_to_rgba(im, output_buffer);
                updateImageTexture(h.texId, output_buffer, im.width(), im.height());
                delete[] output_buffer;

                images.emplace(path, h);
            }
        }
    }

    void get_image_size(const char *src, const char *baseurl, litehtml::size &sz) override
    {
        std::string path;
        if (baseurl != nullptr)
            path += baseurl;
        if (src != nullptr)
            path += src;

        if (images.count(path))
        {
            sz.height = images[path].height;
            sz.width = images[path].width;
        }
        //  logger->error("TODOIMGSIZE");
    }

    void draw_image(litehtml::uint_ptr hdc, const litehtml::background_layer &layer, const litehtml::string &url, const litehtml::string &base_url) override
    {
        // logger->error("TODOIMGDRAW");
        std::string path;
        path += base_url;
        path += url;

        if (images.count(path))
        {
            auto p = ImGui::GetWindowPos();
            auto pos = layer.border_box;
            float pos_x = p.x + pos.x;
            float pos_y = p.y + pos.y;
            ImGui::GetWindowDrawList()->AddImage((void *)(intptr_t)images[path].texId, {pos_x, pos_y}, {pos_x + pos.width, pos_y + pos.height});
        }
    }

    void draw_solid_fill(litehtml::uint_ptr hdc, const litehtml::background_layer &layer, const litehtml::web_color &color) override
    {
        auto p = ImGui::GetWindowPos();
        ImGui::GetWindowDrawList()->AddRectFilled({p.x + (float)layer.border_box.x, p.y + (float)layer.border_box.y},
                                                  {p.x + (float)layer.border_box.x + layer.border_box.width, p.y + (float)layer.border_box.y + layer.border_box.height},
                                                  ImColor(color.red, color.green, color.blue, color.alpha));
    }

    void draw_linear_gradient(litehtml::uint_ptr hdc, const litehtml::background_layer &layer, const litehtml::background_layer::linear_gradient &gradient) override {}
    void draw_radial_gradient(litehtml::uint_ptr hdc, const litehtml::background_layer &layer, const litehtml::background_layer::radial_gradient &gradient) override {}
    void draw_conic_gradient(litehtml::uint_ptr hdc, const litehtml::background_layer &layer, const litehtml::background_layer::conic_gradient &gradient) override {}

    void draw_borders(litehtml::uint_ptr hdc, const litehtml::borders &borders, const litehtml::position &draw_pos, bool root) override
    {
        auto p = ImGui::GetWindowPos();

        ImGui::GetWindowDrawList()->AddRect({p.x + (float)draw_pos.x, p.y + (float)draw_pos.y}, {p.x + (float)draw_pos.x + draw_pos.width, p.y + (float)draw_pos.y + draw_pos.height},
                                            ImColor(borders.bottom.color.red, borders.bottom.color.green, borders.bottom.color.blue, borders.bottom.color.alpha));
    }

    void draw_list_marker(litehtml::uint_ptr hdc, const litehtml::list_marker &marker) override
    {
        //
    }

    litehtml::element::ptr create_element(const char * /*tag_name*/, const litehtml::string_map & /*attributes*/, const litehtml::document::ptr & /*doc*/) override { return nullptr; }

    void get_media_features(litehtml::media_features &media) const override
    {
        //
    }
    void get_language(litehtml::string & /*language*/, litehtml::string & /*culture*/) const override
    {
        //
    }
    void link(const litehtml::document::ptr & /*doc*/, const litehtml::element::ptr & /*el*/) override
    {
        //
    }

    void transform_text(litehtml::string & /*text*/, litehtml::text_transform /*tt*/) override {}
    void set_clip(const litehtml::position & /*pos*/, const litehtml::border_radiuses & /*bdr_radius*/) override {}
    void del_clip() override {}

    void set_caption(const char * /*caption*/) override {}
    void set_base_url(const char * /*base_url*/) override {}
    void on_anchor_click(const char * /*url*/, const litehtml::element::ptr & /*el*/) override {}
    void on_mouse_event(const litehtml::element::ptr & /*el*/, litehtml::mouse_event /*event*/) override {};
    void set_cursor(const char * /*cursor*/) override {}

    void import_css(litehtml::string &text, const litehtml::string &url, litehtml::string &baseurl) override
    {
        // TODO
        std::string path = baseurl + url;
        text = satdump::loadFileToString("documentation/html/" + path);
    }

    void get_client_rect(litehtml::position &client) const /*override*/ { client = {0, 0, width, height}; }

    void get_viewport(litehtml::position &viewport) const override { viewport = {0, 0, width, height}; }
};
