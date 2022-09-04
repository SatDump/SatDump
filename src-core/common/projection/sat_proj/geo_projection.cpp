#include "geo_projection.h"

#include <cmath>
#include "logger.h"

namespace geodetic
{
    namespace projection
    {
        GEOProjector::GEOProjector(double sat_lon,
                                   double sat_height,
                                   int img_width,
                                   int img_height,
                                   double hscale,
                                   double vscale,
                                   double x_offset,
                                   double y_offset,
                                   bool sweep_x) : hscale(hscale),
                                                   vscale(vscale),
                                                   x_offset(x_offset),
                                                   y_offset(y_offset)
        {
            height = img_height;
            width = img_width;
            pj.init(sat_height * 1000, sat_lon, sweep_x);
        }

        int GEOProjector::forward(double lon, double lat, int &img_x, int &img_y)
        {
            if (pj.forward(lon, lat, x, y))
            {
                // Error / out of the image
                img_x = -1;
                img_y = -1;
                return 1;
            }

            image_x = x * hscale * (width / 2.0);
            image_y = y * vscale * (height / 2.0);

            image_x += width / 2.0 + x_offset;
            image_y += height / 2.0 + y_offset;

            img_x = image_x;
            img_y = (height - 1) - image_y;

            if (img_x >= width || img_y >= height)
                return 1;

            return 0;
        }

        int GEOProjector::inverse(int img_x, int img_y, double &lon, double &lat)
        {
            if (img_x >= width || img_y >= height)
                return 1;

            image_y = (height - 1) - img_y;
            image_x = img_x;

            image_y -= height / 2.0 + y_offset;
            image_x -= width / 2.0 + x_offset;

            y = image_y / (vscale * (height / 2.0));
            x = image_x / (hscale * (width / 2.0));

            if (pj.inverse(x, y, lon, lat))
            {
                // Error / out of the image
                img_x = -1;
                img_y = -1;
                return 1;
            }

            return 0;
        }
    };
};