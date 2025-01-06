#include "map.h"
#include <cmath>
#include "common/geodetic/geodetic_coordinates.h"
#include "common/utils.h"
#include "logger.h"
#include "common/image/io.h"
#include "common/image/meta.h"

std::pair<double, double> deg2num(double lat, double lon, int zoom)
{
    double n = pow(2, zoom);
    double xtile = n * ((lon + 180.0) / 360.0);
    double ytile = (1 - asinh(tan(lat * DEG_TO_RAD)) / M_PI) * (n / 2.0);
    return {xtile, ytile};
}

std::pair<double, double> from4326_to3857(double lat, double lon)
{
    const double EARTH_EQUATORIAL_RADIUS = 6378137.0;
    double xtile = (lon * DEG_TO_RAD) * EARTH_EQUATORIAL_RADIUS;
    double ytile = log(tan(DEG_TO_RAD * (45 + lat / 2.0))) * EARTH_EQUATORIAL_RADIUS;
    return {xtile, ytile};
}

// struct MapTile
//{
//     int z, x, y;
//     image::Image img;
// };

image::Image downloadTileMap(std::string url_source, double lat0, double lon0, double lat1, double lon1, int zoom)
{
    std::pair<double, double> xy0 = deg2num(lat0, lon0, zoom);
    std::pair<double, double> xy1 = deg2num(lat1, lon1, zoom);
    if (xy0.first > xy1.first)
    {
        auto tmp = xy0.first;
        xy0.first = xy1.first;
        xy1.first = tmp;
    }
    if (xy0.second > xy1.second)
    {
        auto tmp = xy0.second;
        xy0.second = xy1.second;
        xy1.second = tmp;
    }

    double bbox[4] = {floor(xy0.first), floor(xy0.second), ceil(xy1.first), ceil(xy1.second)};

    int x_size = bbox[2] - bbox[0]; // xy1.first - xy0.first;
    int y_size = bbox[3] - bbox[1]; // xy1.second - xy0.second;

    logger->trace("Tile map will be of size %dx%d (%f, %f => %f, %f)",
                  x_size, y_size,
                  xy0.first, xy0.second,
                  xy1.first, xy1.second);
    image::Image image_output(8, x_size * 256, y_size * 256, 3);

    for (int x = bbox[0]; x < bbox[2]; x++)
    {
        for (int y = bbox[1]; y < bbox[3]; y++)
        {
            std::string url = url_source;
            std::string res;

            url.replace(url.find("{z}"), 3, std::to_string(zoom));
            url.replace(url.find("{x}"), 3, std::to_string(x));
            url.replace(url.find("{y}"), 3, std::to_string(y));

            try
            {
                logger->debug("Downloading tile from: " + url);

                perform_http_request(url, res);

                image::Image tile;
                image::load_img(tile, (uint8_t *)res.data(), res.size());
                if (tile.depth() != 8)
                    tile = tile.to8bits();

                int tilepos_x = x - bbox[0];
                int tilepos_y = y - bbox[1];

                image_output.draw_image(0, tile, tilepos_x * 256, tilepos_y * 256);
            }
            catch (std::exception &e)
            {
                logger->error("Error downloading tile from: " + url + " %s", e.what());
            }
        }
    }

    double base_size[2] = {256, 256};
    double xfrac = xy0.first - bbox[0];
    double yfrac = xy0.second - bbox[1];
    double x2 = round(base_size[0] * xfrac);
    double y2 = round(base_size[1] * yfrac);
    double imgw = round(base_size[0] * (xy1.first - xy0.first));
    double imgh = round(base_size[1] * (xy1.second - xy0.second));

    image_output.crop(x2, y2, x2 + imgw, y2 + imgh);

    std::pair<double, double> xyp0 = from4326_to3857(lat0, lon0);
    std::pair<double, double> xyp1 = from4326_to3857(lat1, lon1);
    double pwidth = abs(xyp1.first - xyp0.first) / (double)image_output.width();
    double pheight = abs(xyp1.second - xyp0.second) / (double)image_output.height();

    nlohmann::json proj_cfg;
    proj_cfg["type"] = "webmerc";
    proj_cfg["offset_x"] = std::min(xyp0.first, xyp1.first);
    proj_cfg["offset_y"] = std::max(xyp0.second, xyp1.second);
    proj_cfg["scalar_x"] = pwidth;
    proj_cfg["scalar_y"] = -pheight;
    image::set_metadata_proj_cfg(image_output, proj_cfg);

    return image_output;
}
