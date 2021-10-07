#include "project.h"
#include "logger.h"
#include "common/projection/leo_to_equirect.h"
#include "common/projection/proj_file.h"
#include "common/map/map_drawer.h"
#include "resources.h"

#include "common/projection/stereo.h"
#include "common/projection/geos.h"
#include "common/map/maidenhead.h"

class GEOProjector
{
private:
    projection::GEOSProjection pj;
    double height, width;

    double x, y;
    double image_x, image_y;

public:
    double hscale = 1;
    double vscale = 1;
    double x_offset = 0;
    double y_offset = 0;

    GEOProjector(double sat_lon, double sat_height, int img_width, int img_height)
    {
        height = img_height;
        width = img_width;

        pj.init(sat_height * 1000, sat_lon, true);
    }

    int forward(double lon, double lat, int &img_x, int &img_y)
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

        return 0;
    }

    int inverse(int img_x, int img_y, double &lon, double &lat)
    {
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

void projectGEOToproj(cimg_library::CImg<unsigned short> image, cimg_library::CImg<unsigned char> &projected_image, int channels, projection::proj_file::GEO_GeodeticReferenceFile refFile, std::function<std::pair<int, int>(float, float, int, int)> toMapCoords)
{
    GEOProjector projector(refFile.position_longitude, refFile.position_height, image.width(), image.height());
    projector.hscale = refFile.horizontal_scale;
    projector.vscale = refFile.vertical_scale;
    projector.x_offset = refFile.horizontal_offset;
    projector.y_offset = refFile.vertical_offset;

    // Reproject
    /*for (int currentScan = 0; currentScan < (int)image.height(); currentScan++)
    {
        // Now compute each pixel's lat / lon and plot it
        for (double px = 0; px < image.width() - 1; px++)
        {
            double lat1, lon1, lat2, lon2;
            int ret1 = projector.inverse(px, currentScan, lon1, lat1);
            int ret2 = projector.inverse(px + 1, currentScan, lon2, lat2);

            if (ret1 || ret2)
                continue;

            std::pair<float, float> map_cc1 = toMapCoords(lat1, lon1, projected_image.height(), projected_image.width());
            std::pair<float, float> map_cc2 = toMapCoords(lat2, lon2, projected_image.height(), projected_image.width());

            unsigned char color[3];
            if (channels == 3)
            {
                color[0] = image[image.width() * image.height() * 0 + currentScan * image.width() + int(px)] >> 8;
                color[1] = image[image.width() * image.height() * 1 + currentScan * image.width() + int(px)] >> 8;
                color[2] = image[image.width() * image.height() * 2 + currentScan * image.width() + int(px)] >> 8;
            }
            else
            {
                color[0] = image[currentScan * image.width() + int(px)] >> 8;
                color[1] = image[currentScan * image.width() + int(px)] >> 8;
                color[2] = image[currentScan * image.width() + int(px)] >> 8;
            }

            if (color[0] == 0 && color[1] == 0 && color[2] == 0) // Skip Black
                continue;

            // This seems to glitch out sometimes... Need to check
            if (abs(map_cc1.first - map_cc2.first) < 100 && abs(map_cc1.second - map_cc2.second) < 100)
            {
                double circle_radius = sqrt(pow(int(map_cc1.first - map_cc2.first), 2) + pow(int(map_cc1.second - map_cc2.second), 2));
                projected_image.draw_circle(map_cc1.first, map_cc1.second, ceil(circle_radius), color, 0.4);
            }

            //if (abs(map_cc1.first - map_cc2.first) < 20 && abs(map_cc1.second - map_cc2.second) < 20)
            //    projected_image.draw_point(map_cc1.first, map_cc1.second, color);

            //if (map_cc1.first == map_cc2.first && map_cc1.second == map_cc2.second)
            //    projected_image.draw_point(map_cc1.first, map_cc1.second, color);
            //else if (abs(map_cc1.first - map_cc2.first) < 20 && abs(map_cc1.second - map_cc2.second) < 20)
            //    projected_image.draw_rectangle(map_cc1.first, map_cc1.second, map_cc2.first, map_cc2.second, color);
        }

        //logger->info(std::to_string(currentScan));

        //logger->critical(std::to_string(currentScan));
    }*/

    for (double lat = -90; lat < 90; lat += 0.01)
    {
        for (double lon = -180; lon < 180; lon += 0.01)
        {
            int x, y;
            if (projector.forward(lon, lat, x, y))
                continue;

            std::pair<float, float> map_cc1 = toMapCoords(lat, lon, projected_image.height(), projected_image.width());

            unsigned char color[3];
            if (channels == 3)
            {
                color[0] = image[image.width() * image.height() * 0 + y * image.width() + int(x)] >> 8;
                color[1] = image[image.width() * image.height() * 1 + y * image.width() + int(x)] >> 8;
                color[2] = image[image.width() * image.height() * 2 + y * image.width() + int(x)] >> 8;
            }
            else
            {
                color[0] = image[y * image.width() + int(x)] >> 8;
                color[1] = image[y * image.width() + int(x)] >> 8;
                color[2] = image[y * image.width() + int(x)] >> 8;
            }

            if (color[0] == 0 && color[1] == 0 && color[2] == 0) // Skip Black
                continue;

            if (color[0] >= 253 && color[1] >= 253 && color[2] >= 253) // Skip Full white, as it's usually filler on GEO (eg, xRIT)
                continue;

            //logger->info(std::to_string(color[0]) + " " + std::to_string(color[1]) + " " + std::to_string(color[2]));

            projected_image.draw_point(map_cc1.first, map_cc1.second, color);
        }
        logger->info(lat);
    }

    unsigned char color[3] = {0, 255, 0};

    map::drawProjectedMapShapefile(projected_image,
                                   {resources::getResourcePath("maps/ne_10m_admin_0_countries.shp")},
                                   color,
                                   toMapCoords);
}

int project(int argc, char *argv[])
{
    logger->critical("This is EXTREMELY WIP! You have been warned!");

    if (argc < 4)
    {
        logger->error("Usage : project image_height image_width [equirectangular/stereo/geos]. Run that command to get per-projection usage.");
        return 1;
    }

    std::string proj = argv[4];

    if (argc < 6)
    {
        if (proj == "equirectangular")
            logger->error("Usage : project image_height image_width equirectangular output.png image1.png reference1.georef image2.png reference2.georef...");
        else if (proj == "stereo")
            logger->error("Usage : project image_height image_width stereo center_locator projection_scale output.png image.png reference.georef image2.png reference2.georef...");
        else if (proj == "geos")
            logger->error("Usage : project image_height image_width geos satellite_longitude output.png image.png reference.georef image2.png reference2.georef...");
        else
            logger->error("Invalid projection!");
        return 1;
    }

    projection::StereoProjection proj_stereo;
    projection::GEOSProjection proj_geos(30000000, -0);

    std::function<std::pair<int, int>(float, float, int, int)> projectionFunc = [](float lat, float lon, int map_height, int map_width) -> std::pair<int, int>
    {
        int imageLat = map_height - ((90.0f + lat) / 180.0f) * map_height;
        int imageLon = (lon / 360.0f) * map_width + (map_width / 2);
        return {imageLon, imageLat};
    };

    int image_height = std::stoi(argv[1]);
    int image_width = std::stoi(argv[2]);
    std::string output_name = argv[3];

    if (proj == "stereo")
    {
        std::string locator = argv[5];
        float scale = std::stof(argv[6]);
        logger->info("Projecting to Stereographic centered around " + locator);
        proj_stereo.init(mh2lat((char *)locator.c_str()), mh2lon((char *)locator.c_str()));
        // StereoProj
        projectionFunc = [&proj_stereo, scale](float lat, float lon, int map_height, int map_width) -> std::pair<int, int>
        {
            double x, y;
            proj_stereo.forward(lon, lat, x, y);
            x *= map_width / scale;
            y *= map_height / scale;
            return {x + (map_width / 2), map_height - (y + (map_height / 2))};
        };

        argc -= 3;
        argv = &argv[3];
    }
    else if (proj == "geos")
    {
        logger->info("Projecting to Geostationnary point of view at " + std::string(argv[5]) + " longitude.");
        proj_geos.init(30000000, std::stof(argv[5]));
        // Geos
        projectionFunc = [&proj_geos](float lat, float lon, int map_height, int map_width) -> std::pair<int, int>
        {
            double x, y;
            proj_geos.forward(lon, lat, x, y);
            x *= map_width / 2;
            y *= map_height / 2;
            return {x + (map_width / 2), map_height - (y + (map_height / 2))};
        };

        argc -= 2;
        argv = &argv[2];
    }
    else
    {
        argc -= 1;
        argv = &argv[1];
    }

    std::vector<std::pair<std::string, std::string>> toProject;
    int count = (argc - 3) - ((argc - 3) % 2);
    for (int i = 0; i < count; i += 2)
        toProject.push_back({std::string(argv[i + 4 + 0]), std::string(argv[i + 4 + 1])});

    cimg_library::CImg<unsigned char> projected_image = cimg_library::CImg<unsigned char>(image_width, image_height, 1, 3, 0);

    for (const std::pair<std::string, std::string> &image : toProject)
    {
        logger->info("Projecting " + image.first + "...");
        cimg_library::CImg<unsigned short> src_image(image.first.c_str());
        std::shared_ptr<projection::proj_file::GeodeticReferenceFile> geofile = projection::proj_file::readReferenceFile(image.second);

        if (geofile->file_type == projection::proj_file::LEO_TYPE)
        {
            projection::proj_file::LEO_GeodeticReferenceFile leofile = *((projection::proj_file::LEO_GeodeticReferenceFile *)geofile.get());
            projection::LEOScanProjectorSettings settings = leoProjectionRefFile(leofile);
            projection::LEOScanProjector projector(settings);
            logger->info("Reprojecting LEO...");
            projected_image = projection::projectLEOToEquirectangularMapped(src_image, projector, image_width, image_height, src_image.spectrum(), projected_image, projectionFunc);
        }
        else if (geofile->file_type == projection::proj_file::GEO_TYPE)
        {
            src_image.normalize(0, 65535);
            projection::proj_file::GEO_GeodeticReferenceFile gsofile = *((projection::proj_file::GEO_GeodeticReferenceFile *)geofile.get());
            src_image.resize(gsofile.image_width, gsofile.image_height); // Safety
            logger->info("Reprojecting GEO...");
            projectGEOToproj(src_image, projected_image, src_image.spectrum(), gsofile, projectionFunc);
        }
    }

    unsigned char color[3] = {255, 0, 0};

    map::drawProjectedCapitalsGeoJson(projected_image, {resources::getResourcePath("maps/ne_10m_populated_places_simple.json")}, color, projectionFunc);

    logger->info("Saving...");
    projected_image.save_png(output_name.c_str());

    return 0;
}