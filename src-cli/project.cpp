#include "project.h"
#include "logger.h"
#include "common/geodetic/projection/satellite_reprojector.h"
#include "common/geodetic/projection/proj_file.h"
#include "common/map/map_drawer.h"
#include "resources.h"

#include "common/geodetic/projection/stereo.h"
#include "common/geodetic/projection/geos.h"
#include "common/map/maidenhead.h"

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

    geodetic::projection::StereoProjection proj_stereo;
    geodetic::projection::GEOSProjection proj_geos(30000000, -0);

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

    image::Image<uint8_t> projected_image = image::Image<uint8_t>(image_width, image_height, 3);

    for (const std::pair<std::string, std::string> &image : toProject)
    {
        logger->info("Projecting " + image.first + "...");
        image::Image<uint8_t> src_image;
        src_image.load_png(image.first);
        std::shared_ptr<geodetic::projection::proj_file::GeodeticReferenceFile> geofile = geodetic::projection::proj_file::readReferenceFile(image.second);

        if (geofile->file_type == 0)
        {
            logger->info("Reprojecting Equiectangular...");
            geodetic::projection::projectEQUIToproj(src_image, projected_image, src_image.channels(), projectionFunc);
        }
        else if (geofile->file_type == geodetic::projection::proj_file::LEO_TYPE)
        {
            geodetic::projection::proj_file::LEO_GeodeticReferenceFile leofile = *((geodetic::projection::proj_file::LEO_GeodeticReferenceFile *)geofile.get());
            std::shared_ptr<geodetic::projection::LEOScanProjectorSettings> settings = leoProjectionRefFile(leofile);
            geodetic::projection::LEOScanProjector projector(settings);
            logger->info("Reprojecting LEO...");
            geodetic::projection::reprojectLEOtoProj(src_image, projector, projected_image, src_image.channels(), projectionFunc);
        }
        else if (geofile->file_type == geodetic::projection::proj_file::GEO_TYPE)
        {
            geodetic::projection::proj_file::GEO_GeodeticReferenceFile gsofile = *((geodetic::projection::proj_file::GEO_GeodeticReferenceFile *)geofile.get());
            src_image.resize(gsofile.image_width, gsofile.image_height); // Safety
            logger->info("Reprojecting GEO...");
            geodetic::projection::GEOProjector projector = geodetic::projection::proj_file::geoProjectionRefFile(gsofile);
            geodetic::projection::reprojectGEOtoProj(src_image, projector, projected_image, src_image.channels(), projectionFunc);
        }
    }

    unsigned char color[3] = {0, 255, 0};
    map::drawProjectedMapShapefile({resources::getResourcePath("maps/ne_10m_admin_0_countries.shp")}, projected_image, color, projectionFunc);

    unsigned char color2[3] = {255, 0, 0};
    map::drawProjectedCapitalsGeoJson({resources::getResourcePath("maps/ne_10m_populated_places_simple.json")}, projected_image, color2, projectionFunc);

    logger->info("Saving...");
    projected_image.save_png(output_name.c_str());

    return 0;
}