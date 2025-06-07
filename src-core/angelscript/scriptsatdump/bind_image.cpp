#include "angelscript/angelscript.h"
#include "angelscript/scriptarray/scriptarray.h"
#include "bind_satdump.h"

#include "image/image.h"
#include "image/io.h"
#include "image/processing.h"

#include "bind_json.h"
#include "image/meta.h"
#include "image/text.h"

#include "bind_image.h"

namespace satdump
{
    namespace script
    {
        std::vector<double> arrayToVectorDouble(CScriptArray *array) { return std::vector<double>((double *)array->At(0), (double *)array->At(0) + array->GetSize()); }

        namespace img
        {
            void to_rgb(void *ptr) { ((CScriptImage *)ptr)->img->to_rgb(); }
            void to_rgba(void *ptr) { ((CScriptImage *)ptr)->img->to_rgba(); }
            CScriptImage *to8bits(void *ptr) { return new CScriptImage(((CScriptImage *)ptr)->img->to8bits()); }
            CScriptImage *to16bits(void *ptr) { return new CScriptImage(((CScriptImage *)ptr)->img->to16bits()); }
            CScriptImage *to_depth(void *ptr, int bit_depth) { return new CScriptImage(((CScriptImage *)ptr)->img->to_depth(bit_depth)); }

            void draw_image(void *ptr, int channel, CScriptImage *image) { ((CScriptImage *)ptr)->img->draw_image(channel, *image->img); }
            void draw_image2(void *ptr, int channel, CScriptImage *image, int x) { ((CScriptImage *)ptr)->img->draw_image(channel, *image->img, x); }
            void draw_image3(void *ptr, int channel, CScriptImage *image, int x, int y) { ((CScriptImage *)ptr)->img->draw_image(channel, *image->img, x, y); }

            void draw_image_alpha(void *ptr, CScriptImage *image) { ((CScriptImage *)ptr)->img->draw_image_alpha(*image->img); }
            void draw_image_alpha2(void *ptr, CScriptImage *image, int x) { ((CScriptImage *)ptr)->img->draw_image_alpha(*image->img, x); }
            void draw_image_alpha3(void *ptr, CScriptImage *image, int x, int y) { ((CScriptImage *)ptr)->img->draw_image_alpha(*image->img, x, y); }

            void draw_pixel(void *ptr, size_t x, size_t y, CScriptArray *color) { ((CScriptImage *)ptr)->img->draw_pixel(x, y, arrayToVectorDouble(color)); }
            void draw_line(void *ptr, int x0, int y0, int x1, int y1, CScriptArray *color) { ((CScriptImage *)ptr)->img->draw_line(x0, y0, x1, y1, arrayToVectorDouble(color)); }
            void draw_circle(void *ptr, int x0, int y0, int radius, CScriptArray *color) { ((CScriptImage *)ptr)->img->draw_circle(x0, y0, radius, arrayToVectorDouble(color)); }
            void draw_circle2(void *ptr, int x0, int y0, int radius, CScriptArray *color, bool fill) { ((CScriptImage *)ptr)->img->draw_circle(x0, y0, radius, arrayToVectorDouble(color), fill); }
            void draw_rectangle(void *ptr, int x0, int y0, int x1, int y1, CScriptArray *color) { ((CScriptImage *)ptr)->img->draw_rectangle(x0, y0, x1, y1, arrayToVectorDouble(color)); }
            void draw_rectangle2(void *ptr, int x0, int y0, int x1, int y1, CScriptArray *color, bool fill)
            {
                ((CScriptImage *)ptr)->img->draw_rectangle(x0, y0, x1, y1, arrayToVectorDouble(color), fill);
            }

            void init(void *ptr, int bit_depth, size_t width, size_t height, int channels) { ((CScriptImage *)ptr)->img->init(bit_depth, width, height, channels); };
            void clear(void *ptr) { ((CScriptImage *)ptr)->img->clear(); };

            int clamp(void *ptr, int input) { return ((CScriptImage *)ptr)->img->clamp(input); }
            double clampf(void *ptr, double input) { return ((CScriptImage *)ptr)->img->clampf(input); }
            int depth(void *ptr) { return ((CScriptImage *)ptr)->img->depth(); }
            int typesize(void *ptr) { return ((CScriptImage *)ptr)->img->typesize(); }
            size_t width(void *ptr) { return ((CScriptImage *)ptr)->img->width(); }
            size_t height(void *ptr) { return ((CScriptImage *)ptr)->img->height(); }
            int channels(void *ptr) { return ((CScriptImage *)ptr)->img->channels(); }
            size_t size(void *ptr) { return ((CScriptImage *)ptr)->img->size(); }
            int maxval(void *ptr) { return ((CScriptImage *)ptr)->img->maxval(); }

            void crop(void *ptr, int x0, int y0, int x1, int y1) { ((CScriptImage *)ptr)->img->crop(x0, y0, x1, y1); }
            void crop2(void *ptr, int x0, int x1) { ((CScriptImage *)ptr)->img->crop(x0, x1); }
            CScriptImage *crop_to(void *ptr, int x0, int y0, int x1, int y1) { return new CScriptImage(((CScriptImage *)ptr)->img->crop_to(x0, y0, x1, y1)); }
            CScriptImage *crop_to2(void *ptr, int x0, int x1) { return new CScriptImage(((CScriptImage *)ptr)->img->crop_to(x0, x1)); }

            void mirror(void *ptr, bool x, bool y) { ((CScriptImage *)ptr)->img->mirror(x, y); }

            void resize(void *ptr, int width, int height) { ((CScriptImage *)ptr)->img->resize(width, height); }
            CScriptImage *resize_to(void *ptr, int width, int height) { return new CScriptImage(((CScriptImage *)ptr)->img->resize_to(width, height)); }
            void resize_bilinear(void *ptr, int width, int height) { ((CScriptImage *)ptr)->img->resize_bilinear(width, height); }
            void resize_bilinear2(void *ptr, int width, int height, bool text_mode) { ((CScriptImage *)ptr)->img->resize_bilinear(width, height, text_mode); }
            int get_pixel_bilinear(void *ptr, int channel, double x, double y) { return ((CScriptImage *)ptr)->img->get_pixel_bilinear(channel, x, y); }

            void fill(void *ptr, int val) { ((CScriptImage *)ptr)->img->fill(val); }
            void fill_color(void *ptr, CScriptArray *color) { ((CScriptImage *)ptr)->img->fill_color(arrayToVectorDouble(color)); }

            void set(void *ptr, size_t p, int v) { ((CScriptImage *)ptr)->img->set(p, v); }
            int get(void *ptr, size_t p) { return ((CScriptImage *)ptr)->img->get(p); }
            void setf(void *ptr, size_t p, double v) { return ((CScriptImage *)ptr)->img->setf(p, v); }
            double getf(void *ptr, size_t p) { return ((CScriptImage *)ptr)->img->getf(p); }

            void set2(void *ptr, size_t channel, size_t p, int v) { ((CScriptImage *)ptr)->img->set(channel, p, v); }
            int get2(void *ptr, size_t channel, size_t p) { return ((CScriptImage *)ptr)->img->get(channel, p); }
            void set3(void *ptr, size_t channel, size_t x, size_t y, int v) { ((CScriptImage *)ptr)->img->set(channel, x, y, v); }
            int get3(void *ptr, size_t channel, size_t x, size_t y) { return ((CScriptImage *)ptr)->img->get(channel, x, y); }

            void setf2(void *ptr, size_t channel, size_t p, double v) { ((CScriptImage *)ptr)->img->setf(channel, p, v); }
            double getf2(void *ptr, size_t channel, size_t p) { return ((CScriptImage *)ptr)->img->getf(channel, p); }
            void setf3(void *ptr, size_t channel, size_t x, size_t y, double v) { ((CScriptImage *)ptr)->img->setf(channel, x, y, v); }
            double getf3(void *ptr, size_t channel, size_t x, size_t y) { return ((CScriptImage *)ptr)->img->getf(channel, x, y); }
        } // namespace img

        namespace io
        {
            void load_img(CScriptImage *img, std::string file) { image::load_img(*img->img, file); }
            void save_img(CScriptImage *img, std::string file, bool fast) { image::save_img(*img->img, file, fast); }
        } // namespace io

        namespace proc
        {
            void white_balance(CScriptImage *img) { image::white_balance(*img->img); }
            void white_balance2(CScriptImage *img, float percentileValue) { image::white_balance(*img->img, percentileValue); }

            void median_blur(CScriptImage *img) { image::median_blur(*img->img); }

            void kuwahara_filter(CScriptImage *img) { image::kuwahara_filter(*img->img); }

            void equalize(CScriptImage *img) { image::equalize(*img->img); }
            void equalize2(CScriptImage *img, bool per_channel) { image::equalize(*img->img, per_channel); }

            void normalize(CScriptImage *img) { image::normalize(*img->img); }

            void linear_invert(CScriptImage *img) { image::linear_invert(*img->img); }

            void simple_despeckle(CScriptImage *img) { image::simple_despeckle(*img->img); }
            void simple_despeckle2(CScriptImage *img, int thresold) { image::simple_despeckle(*img->img, thresold); }
        } // namespace proc

        namespace meta
        {
            bool has_metadata(CScriptImage *img) { return image::has_metadata(*img->img); }
            void set_metadata(CScriptImage *img, CScriptJson *metadata) { return image::set_metadata(*img->img, *metadata->j); }
            CScriptJson *get_metadata(CScriptImage *img) { return new CScriptJson(image::get_metadata(*img->img)); }
            void free_metadata(CScriptImage *img) { return image::free_metadata(*img->img); }

            bool has_metadata_proj_cfg(CScriptImage *img) { return image::has_metadata_proj_cfg(*img->img); }
            void set_metadata_proj_cfg(CScriptImage *img, CScriptJson *metadata) { return image::set_metadata_proj_cfg(*img->img, *metadata->j); }
            CScriptJson *get_metadata_proj_cfg(CScriptImage *img) { return new CScriptJson(image::get_metadata_proj_cfg(*img->img)); }
        } // namespace meta

        namespace text
        {
            void TextDrawerConstructor(void *memory) { new (memory) image::TextDrawer(); }
            void TextDrawerDestructor(void *memory) { ((image::TextDrawer *)memory)->~TextDrawer(); }
            void draw_text(void *ptr, CScriptImage *img, int xs0, int ys0, CScriptArray *color, int size, std::string text)
            {
                ((image::TextDrawer *)ptr)->draw_text(*img->img, xs0, ys0, arrayToVectorDouble(color), size, text);
            }
        } // namespace text

        void registerImage(asIScriptEngine *engine)
        {
            engine->SetDefaultNamespace("image");

            ////////////////////////////////
            //////// Core Image Class
            ////////////////////////////////

            // Base type
            engine->RegisterObjectType("Image", sizeof(CScriptImage), asOBJ_REF);

            // Constructors
            engine->RegisterObjectBehaviour("Image", asBEHAVE_FACTORY, "Image@ f()", asFUNCTION(ScriptImageFactory), asCALL_CDECL);
            engine->RegisterObjectBehaviour("Image", asBEHAVE_FACTORY, "Image@ f(int, uint64, uint64, int)", asFUNCTION(ScriptImageFactory2), asCALL_CDECL);

            // Angelscript stuff
            engine->RegisterObjectBehaviour("Image", asBEHAVE_ADDREF, "void f()", asMETHOD(CScriptImage, AddRef), asCALL_THISCALL);
            engine->RegisterObjectBehaviour("Image", asBEHAVE_RELEASE, "void f()", asMETHOD(CScriptImage, Release), asCALL_THISCALL);

            // Copy operator
            engine->RegisterObjectMethod("Image", "Image &opAssign(const Image &in)", asMETHODPR(CScriptImage, operator=, (const CScriptImage &), CScriptImage &), asCALL_THISCALL);

            // To RGB etc...
            engine->RegisterObjectMethod("Image", "void to_rgb()", asFUNCTION(img::to_rgb), asCALL_CDECL_OBJFIRST);
            engine->RegisterObjectMethod("Image", "void to_rgba()", asFUNCTION(img::to_rgba), asCALL_CDECL_OBJFIRST);
            engine->RegisterObjectMethod("Image", "Image@ to8bits()", asFUNCTION(img::to8bits), asCALL_CDECL_OBJFIRST);
            engine->RegisterObjectMethod("Image", "Image@ to16bits()", asFUNCTION(img::to16bits), asCALL_CDECL_OBJFIRST);
            engine->RegisterObjectMethod("Image", "Image@ to_depth(int)", asFUNCTION(img::to_depth), asCALL_CDECL_OBJFIRST);

            // Draws
            engine->RegisterObjectMethod("Image", "void draw_image(int, Image &)", asFUNCTION(img::draw_image), asCALL_CDECL_OBJFIRST);
            engine->RegisterObjectMethod("Image", "void draw_image(int, Image &, int)", asFUNCTION(img::draw_image2), asCALL_CDECL_OBJFIRST);
            engine->RegisterObjectMethod("Image", "void draw_image(int, Image &, int, int)", asFUNCTION(img::draw_image3), asCALL_CDECL_OBJFIRST);
            engine->RegisterObjectMethod("Image", "void draw_image_alpha(Image &)", asFUNCTION(img::draw_image_alpha), asCALL_CDECL_OBJFIRST);
            engine->RegisterObjectMethod("Image", "void draw_image_alpha(Image &, int)", asFUNCTION(img::draw_image_alpha2), asCALL_CDECL_OBJFIRST);
            engine->RegisterObjectMethod("Image", "void draw_image_alpha(Image &, int, int)", asFUNCTION(img::draw_image_alpha3), asCALL_CDECL_OBJFIRST);

            engine->RegisterObjectMethod("Image", "void draw_pixel(uint64, uint64, array<double>@)", asFUNCTION(img::draw_pixel), asCALL_CDECL_OBJFIRST);
            engine->RegisterObjectMethod("Image", "void draw_line(int, int, int, int, array<double>@)", asFUNCTION(img::draw_line), asCALL_CDECL_OBJFIRST);
            engine->RegisterObjectMethod("Image", "void draw_circle(int, int, int, array<double>@)", asFUNCTION(img::draw_circle), asCALL_CDECL_OBJFIRST);
            engine->RegisterObjectMethod("Image", "void draw_circle(int, int, int, array<double>@, bool)", asFUNCTION(img::draw_circle2), asCALL_CDECL_OBJFIRST);
            engine->RegisterObjectMethod("Image", "void draw_rectangle(int, int, int, int, array<double>@)", asFUNCTION(img::draw_rectangle), asCALL_CDECL_OBJFIRST);
            engine->RegisterObjectMethod("Image", "void draw_rectangle(int, int, int, int, array<double>@, bool)", asFUNCTION(img::draw_rectangle2), asCALL_CDECL_OBJFIRST);

            // Init, clear
            engine->RegisterObjectMethod("Image", "void init(int, uint64, uint64, int)", asFUNCTION(img::init), asCALL_CDECL_OBJFIRST);
            engine->RegisterObjectMethod("Image", "void clear()", asFUNCTION(img::clear), asCALL_CDECL_OBJFIRST);

            // Clamp, width/height/etc
            engine->RegisterObjectMethod("Image", "int clamp(int)", asFUNCTION(img::clamp), asCALL_CDECL_OBJFIRST);
            engine->RegisterObjectMethod("Image", "double clampf(double)", asFUNCTION(img::clampf), asCALL_CDECL_OBJFIRST);
            engine->RegisterObjectMethod("Image", "int depth()", asFUNCTION(img::depth), asCALL_CDECL_OBJFIRST);
            engine->RegisterObjectMethod("Image", "int typesize()", asFUNCTION(img::typesize), asCALL_CDECL_OBJFIRST);
            engine->RegisterObjectMethod("Image", "uint64 width()", asFUNCTION(img::width), asCALL_CDECL_OBJFIRST);
            engine->RegisterObjectMethod("Image", "uint64 height()", asFUNCTION(img::height), asCALL_CDECL_OBJFIRST);
            engine->RegisterObjectMethod("Image", "int channels()", asFUNCTION(img::channels), asCALL_CDECL_OBJFIRST);
            engine->RegisterObjectMethod("Image", "uint64 size()", asFUNCTION(img::size), asCALL_CDECL_OBJFIRST);
            engine->RegisterObjectMethod("Image", "int maxval()", asFUNCTION(img::maxval), asCALL_CDECL_OBJFIRST);

            // Crops
            engine->RegisterObjectMethod("Image", "void crop(int, int, int, int)", asFUNCTION(img::crop), asCALL_CDECL_OBJFIRST);
            engine->RegisterObjectMethod("Image", "void crop(int, int)", asFUNCTION(img::crop2), asCALL_CDECL_OBJFIRST);
            engine->RegisterObjectMethod("Image", "Image@ crop_to(int, int, int, int)", asFUNCTION(img::crop_to), asCALL_CDECL_OBJFIRST);
            engine->RegisterObjectMethod("Image", "Image@ crop_to(int, int)", asFUNCTION(img::crop_to2), asCALL_CDECL_OBJFIRST);

            // Mirror
            engine->RegisterObjectMethod("Image", "void mirror(bool, bool)", asFUNCTION(img::mirror), asCALL_CDECL_OBJFIRST);

            // Resize
            engine->RegisterObjectMethod("Image", "void resize(int, int)", asFUNCTION(img::resize), asCALL_CDECL_OBJFIRST);
            engine->RegisterObjectMethod("Image", "Image@ resize_to(int, int)", asFUNCTION(img::resize_to), asCALL_CDECL_OBJFIRST);
            engine->RegisterObjectMethod("Image", "void resize_bilinear(int, int, bool)", asFUNCTION(img::resize_bilinear), asCALL_CDECL_OBJFIRST);
            engine->RegisterObjectMethod("Image", "int get_pixel_bilinear(int, double, double)", asFUNCTION(img::get_pixel_bilinear), asCALL_CDECL_OBJFIRST);

            // Fill
            engine->RegisterObjectMethod("Image", "void fill(int)", asFUNCTION(img::fill), asCALL_CDECL_OBJFIRST);
            engine->RegisterObjectMethod("Image", "void fill_color(array<double>@)", asFUNCTION(img::fill_color), asCALL_CDECL_OBJFIRST);

            // Set, get
            engine->RegisterObjectMethod("Image", "void set(uint64, int)", asFUNCTION(img::set), asCALL_CDECL_OBJFIRST);
            engine->RegisterObjectMethod("Image", "void set(uint64, uint64, int)", asFUNCTION(img::set2), asCALL_CDECL_OBJFIRST);
            engine->RegisterObjectMethod("Image", "void set(uint64, uint64, uint64, int)", asFUNCTION(img::set3), asCALL_CDECL_OBJFIRST);
            engine->RegisterObjectMethod("Image", "int get(uint64)", asFUNCTION(img::get), asCALL_CDECL_OBJFIRST);
            engine->RegisterObjectMethod("Image", "int get(uint64, uint64)", asFUNCTION(img::get2), asCALL_CDECL_OBJFIRST);
            engine->RegisterObjectMethod("Image", "int get(uint64, uint64, uint64)", asFUNCTION(img::get3), asCALL_CDECL_OBJFIRST);
            engine->RegisterObjectMethod("Image", "void setf(uint64, double)", asFUNCTION(img::setf), asCALL_CDECL_OBJFIRST);
            engine->RegisterObjectMethod("Image", "void setf(uint64, uint64, double)", asFUNCTION(img::setf2), asCALL_CDECL_OBJFIRST);
            engine->RegisterObjectMethod("Image", "void setf(uint64, uint64, uint64, double)", asFUNCTION(img::setf3), asCALL_CDECL_OBJFIRST);
            engine->RegisterObjectMethod("Image", "double getf(uint64)", asFUNCTION(img::getf), asCALL_CDECL_OBJFIRST);
            engine->RegisterObjectMethod("Image", "double getf(uint64, uint64)", asFUNCTION(img::getf2), asCALL_CDECL_OBJFIRST);
            engine->RegisterObjectMethod("Image", "double getf(uint64, uint64, uint64)", asFUNCTION(img::getf3), asCALL_CDECL_OBJFIRST);

            ////////////////////////////////
            //////// Image IO
            ////////////////////////////////

            // TODOREWORK
            engine->RegisterGlobalFunction("void load_img(Image &, string)", asFUNCTION(io::load_img), asCALL_CDECL);
            engine->RegisterGlobalFunction("void save_img(Image &, string, bool)", asFUNCTION(io::save_img), asCALL_CDECL);

            ////////////////////////////////
            //////// Image Processing
            ////////////////////////////////

            engine->RegisterGlobalFunction("void white_balance(Image &)", asFUNCTION(proc::white_balance), asCALL_CDECL);
            engine->RegisterGlobalFunction("void white_balance(Image &, float)", asFUNCTION(proc::white_balance2), asCALL_CDECL);

            engine->RegisterGlobalFunction("void median_blur(Image &)", asFUNCTION(proc::median_blur), asCALL_CDECL);

            engine->RegisterGlobalFunction("void kuwahara_filter(Image &)", asFUNCTION(proc::kuwahara_filter), asCALL_CDECL);

            engine->RegisterGlobalFunction("void equalize(Image &)", asFUNCTION(proc::equalize), asCALL_CDECL);
            engine->RegisterGlobalFunction("void equalize(Image &, bool)", asFUNCTION(proc::equalize2), asCALL_CDECL);

            engine->RegisterGlobalFunction("void normalize(Image &)", asFUNCTION(proc::normalize), asCALL_CDECL);

            engine->RegisterGlobalFunction("void linear_invert(Image &)", asFUNCTION(proc::linear_invert), asCALL_CDECL);

            engine->RegisterGlobalFunction("void simple_despeckle(Image &)", asFUNCTION(proc::simple_despeckle), asCALL_CDECL);
            engine->RegisterGlobalFunction("void simple_despeckle(Image &, int)", asFUNCTION(proc::simple_despeckle2), asCALL_CDECL);

            ////////////////////////////////
            //////// Image Meta
            ////////////////////////////////

            engine->RegisterGlobalFunction("bool has_metadata(Image &)", asFUNCTION(meta::has_metadata), asCALL_CDECL);
            engine->RegisterGlobalFunction("void set_metadata(Image &, nlohmann::json &)", asFUNCTION(meta::set_metadata), asCALL_CDECL);
            engine->RegisterGlobalFunction("nlohmann::json@ get_metadata(Image &)", asFUNCTION(meta::get_metadata), asCALL_CDECL);
            engine->RegisterGlobalFunction("void free_metadata(Image &)", asFUNCTION(meta::free_metadata), asCALL_CDECL);

            engine->RegisterGlobalFunction("bool has_metadata_proj_cfg(Image &)", asFUNCTION(meta::has_metadata_proj_cfg), asCALL_CDECL);
            engine->RegisterGlobalFunction("void set_metadata_proj_cfg(Image &, nlohmann::json &)", asFUNCTION(meta::set_metadata_proj_cfg), asCALL_CDECL);
            engine->RegisterGlobalFunction("nlohmann::json@ get_metadata_proj_cfg(Image &)", asFUNCTION(meta::get_metadata_proj_cfg), asCALL_CDECL);

            ////////////////////////////////
            //////// Image Text
            ////////////////////////////////

            engine->RegisterObjectType("TextDrawer", sizeof(image::TextDrawer), asOBJ_VALUE | asGetTypeTraits<image::TextDrawer>());
            engine->RegisterObjectBehaviour("TextDrawer", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(text::TextDrawerConstructor), asCALL_CDECL_OBJLAST);
            engine->RegisterObjectBehaviour("TextDrawer", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(text::TextDrawerDestructor), asCALL_CDECL_OBJLAST);

            engine->RegisterObjectMethod("TextDrawer", "void init_font(string &in)", asMETHODPR(image::TextDrawer, init_font, (std::string), void), asCALL_THISCALL);
            engine->RegisterObjectMethod("TextDrawer", "void init_font()", asMETHODPR(image::TextDrawer, init_font, (), void), asCALL_THISCALL);
            engine->RegisterObjectMethod("TextDrawer", "bool font_ready()", asMETHOD(image::TextDrawer, font_ready), asCALL_THISCALL);
            engine->RegisterObjectMethod("TextDrawer", "void draw_text(Image &, int, int, array<double>@, int, string)", asFUNCTION(text::draw_text), asCALL_CDECL_OBJFIRST);
        }
    } // namespace script
} // namespace satdump