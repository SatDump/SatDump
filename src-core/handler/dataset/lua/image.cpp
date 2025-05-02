#include "common/image/image.h"
#include "common/image/io.h"
#include "common/image/meta.h"
#include "common/image/processing.h"
#include "lua_bind.h"
#include "projection/projection.h"

namespace satdump
{
    namespace lua
    {
        void bind_image(sol::state &lua)
        {
            sol::usertype<image::Image> image_type = lua.new_usertype<image::Image>("Image", sol::constructors<image::Image(), image::Image(int, size_t, size_t, int)>());

            image_type["clear"] = &image::Image::clear;

            image_type["get"] = [](image::Image &img, size_t i) -> int
            {
                if (i < img.size())
                    return img.get(i);
                else
                    return 0;
            };
            image_type["set"] = [](image::Image &img, size_t i, int x)
            {
                if (i < img.size())
                    img.set(i, img.clamp(x));
            };

            image_type["depth"] = &image::Image::depth;
            image_type["width"] = &image::Image::width;
            image_type["height"] = &image::Image::height;
            image_type["channels"] = &image::Image::channels;
            image_type["size"] = &image::Image::size;

            image_type["to_rgb"] = &image::Image::to_rgb;
            image_type["to_rgba"] = &image::Image::to_rgba;

            image_type["fill_color"] = &image::Image::fill_color;
            image_type["fill"] = &image::Image::fill;
            image_type["mirror"] = &image::Image::mirror;
            lua["image_equalize"] = &image::equalize;
            lua["image_white_balance"] = &image::white_balance;

            image_type["resize"] = &image::Image::resize;
            image_type["resize_to"] = &image::Image::resize_to;
            image_type["resize_bilinear"] = &image::Image::resize_bilinear;

            image_type["draw_pixel"] = &image::Image::draw_pixel;
            image_type["draw_line"] = &image::Image::draw_line;
            image_type["draw_circle"] = &image::Image::draw_circle;
            image_type["draw_rectangle"] = &image::Image::draw_rectangle;
            image_type["draw_image"] = &image::Image::draw_image;
            image_type["draw_image_alpha"] = &image::Image::draw_image_alpha;

            lua["image_linear_invert"] = &image::linear_invert;
            lua["image_equalize"] = &image::equalize;
            lua["image_simple_despeckle"] = &image::simple_despeckle;
            lua["image_median_blur"] = &image::median_blur;
            lua["image_despeckle"] = &image::kuwahara_filter;

            lua["image_load_img"] = (void (*)(image::Image &, std::string))(&image::load_img);
            lua["image_save_img"] = (void (*)(image::Image &, std::string))(&image::save_img);

            lua["image_get_metadata_proj_cfg"] = [](image::Image &img) { return (proj::Projection)image::get_metadata_proj_cfg(img); };
            lua["image_set_metadata_proj_cfg"] = [](image::Image &img, proj::Projection p) { image::set_metadata_proj_cfg(img, p); };
        }
    } // namespace lua
} // namespace satdump