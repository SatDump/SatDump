#include "image_background.h"
#include "common/geodetic/geodetic_coordinates.h"
#include "core/exception.h"
#include "image/meta.h"
#include "logger.h"
#include "projection/projection.h"

namespace satdump
{
    namespace image
    {
        void remove_background(Image &img, float *progress)
        {
            size_t width = img.width();
            size_t height = img.height();

            nlohmann::json proj_cfg = image::get_metadata_proj_cfg(img);
            proj_cfg["width"] = width;
            proj_cfg["height"] = height;
            satdump::proj::Projection p;
            try
            {
                p = proj_cfg;
                if (!p.init(0, 1))
                    throw satdump_exception("Error setting up proj!");
            }
            catch (std::exception &e)
            {
                logger->warn("Cannot remove background - Failed to setup projection : %s", e.what());
                return;
            }

            img.to_rgba();
            geodetic::geodetic_coords_t dummy; // Hey! I resemble that remark!

            for (size_t y = 0; y < height; y++)
            {
                for (size_t x = 0; x < width; x++)
                    if (p.inverse(x, y, dummy))
                        for (int c = 0; c < img.channels(); c++)
                            img.set(c, y * width + x, 0);

                if (progress != nullptr)
                    *progress = (float)y / (float)height;
            }
        }
    } // namespace image
} // namespace satdump