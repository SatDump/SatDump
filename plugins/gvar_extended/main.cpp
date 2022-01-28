#include "plugin.h"
#include "logger.h"
#include "settings.h"
#include "resources.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>
#include "libs/predict/predict.h"
#include "modules/goes/gvar/module_gvar_image_decoder.h"
#include "tle.h"
#include "common/geodetic/projection/geo_projection.h"
#include "common/map/map_drawer.h"

#define FIXED_FLOAT(x) std::fixed << std::setprecision(3) << (x)

geodetic::projection::GEOSProjection proj_geos;

class GVARExtended : public satdump::Plugin
{
private:
    static std::string misc_preview_text;
    static std::vector<std::array<float, 3>> points;
    static std::vector<std::string> names;

    static void satdumpStartedHandler(const satdump::SatDumpStartedEvent &)
    {
        if (global_cfg.contains("gvar_extended"))
        {
            if (global_cfg["gvar_extended"].contains("preview_misc_text"))
                misc_preview_text = global_cfg["gvar_extended"]["preview_misc_text"].get<std::string>();
            else
                misc_preview_text = "SatDump | GVAR";

            if (global_cfg["gvar_extended"].contains("temperature_points"))
            {
                points.clear();
                for (int i = 0; i < (int)global_cfg["gvar_extended"]["temperature_points"].size(); i++)
                {
                    points.push_back({global_cfg["gvar_extended"]["temperature_points"][i]["lat"].get<float>(),
                                      global_cfg["gvar_extended"]["temperature_points"][i]["lon"].get<float>(),
                                      global_cfg["gvar_extended"]["temperature_points"][i]["radius"].get<float>()});
                    names.push_back(global_cfg["gvar_extended"]["temperature_points"][i]["name"].get<std::string>());
                }
            }
        }
    }

    static void gvarSaveChannelImagesHandler(const goes::gvar::events::GVARSaveChannelImagesEvent &evt)
    {
        logger->info("Preview... preview.png");
        image::Image<uint8_t> preview(1300, 948, 1);
        image::Image<uint8_t> previewImage;

        // Preview generation
        {
            image::Image<uint8_t> channel1_8bit(1040, 948, 1);
            image::Image<uint8_t> channel2_8bit(260, 237, 1);
            image::Image<uint8_t> channel3_8bit(260, 237, 1);
            image::Image<uint8_t> channel4_8bit(260, 237, 1);
            image::Image<uint8_t> channel5_8bit(260, 237, 1);

            image::Image<uint16_t> resized5 = evt.images.image5;
            resized5.resize(1040, 948);

            for (int i = 0; i < (int)channel1_8bit.size(); i++)
            {
                channel1_8bit[i] = resized5[i] / 256;
            }

            image::Image<uint16_t> buff2 = evt.images.image1,
                                               buff3 = evt.images.image2,
                                               buff4 = evt.images.image3,
                                               buff5 = evt.images.image4;

            buff2.resize(260, 237);
            buff3.resize(260, 237);
            buff4.resize(260, 237);
            buff5.resize(260, 237);

            for (int i = 0; i < (int)channel2_8bit.size(); i++)
            {
                channel2_8bit[i] = buff2[i] / 256;
                channel3_8bit[i] = buff3[i] / 256;
                channel4_8bit[i] = buff4[i] / 256;
                channel5_8bit[i] = buff5[i] / 256;
            }
            
            channel1_8bit.simple_despeckle();
            channel2_8bit.simple_despeckle();
            channel3_8bit.simple_despeckle();
            channel4_8bit.simple_despeckle();
            channel5_8bit.simple_despeckle();
            

            preview.draw_image(0, channel1_8bit, 0, 0);
            preview.draw_image(0, channel2_8bit, 1040, 0);
            preview.draw_image(0, channel3_8bit, 1040, 237);
            preview.draw_image(0, channel4_8bit, 1040, 474);
            preview.draw_image(0, channel5_8bit, 1040, 711);
        }

        // Overlay the preview
        {
            std::string sat_name = evt.images.sat_number == 13 ? "EWS-G1 / GOES-13" : ("GOES-" + std::to_string(evt.images.sat_number));
            std::string date_time = (evt.timeReadable->tm_mday > 9 ? std::to_string(evt.timeReadable->tm_mday) : "0" + std::to_string(evt.timeReadable->tm_mday)) + '/' +
                                    (evt.timeReadable->tm_mon + 1 > 9 ? std::to_string(evt.timeReadable->tm_mon + 1) : "0" + std::to_string(evt.timeReadable->tm_mon + 1)) + '/' +
                                    std::to_string(evt.timeReadable->tm_year + 1900) + ' ' +
                                    (evt.timeReadable->tm_hour > 9 ? std::to_string(evt.timeReadable->tm_hour) : "0" + std::to_string(evt.timeReadable->tm_hour)) + ':' +
                                    (evt.timeReadable->tm_min > 9 ? std::to_string(evt.timeReadable->tm_min) : "0" + std::to_string(evt.timeReadable->tm_min)) + " UTC";

            int offsetX, offsetY, bar_height;

            //set ratios for calculating bar size
            float bar_ratio = 0.02;


            bar_height = preview.width() * bar_ratio;
            offsetX = 5; //preview.width() * offsetXratio;
            offsetY = 1; //preview.width() * offsetYratio;

            unsigned char color = 255;

            image::Image<uint8_t> imgtext = image::generate_text_image(sat_name.c_str(), &color, bar_height, offsetX, offsetY); 
            image::Image<uint8_t>imgtext1 = image::generate_text_image(date_time.c_str(), &color, bar_height, offsetX, offsetY); 
            image::Image<uint8_t>imgtext2 = image::generate_text_image(misc_preview_text.c_str(), &color, bar_height, offsetX, offsetY); 

            previewImage = image::Image<uint8_t>(preview.width(), preview.height() + 2 * bar_height, 1);
            previewImage.fill(0);

            previewImage.draw_image(0, imgtext, 0, 0);
            previewImage.draw_image(0, imgtext1, previewImage.width() - imgtext1.width(), 0);
            previewImage.draw_image(0, imgtext2, 0, bar_height + preview.height());
            previewImage.draw_image(0, preview, 0, bar_height);
        }

        previewImage.save_png(std::string(evt.directory + "/preview.png").c_str());

        //calibrated temperature measurement based on NOAA LUTs (https://www.ospo.noaa.gov/Operations/GOES/calibration/gvar-conversion.html)
        if (evt.images.image1.width() == 5206 || evt.images.image1.width() == 5209)
        {
            std::string filename = "goes/gvar/goes" + std::to_string(evt.images.sat_number) + "_gvar_lut.txt";
            if (resources::resourceExists(filename) && points.size() > 0)
            {
                std::ifstream input(resources::getResourcePath(filename).c_str());
                std::array<std::array<float, 1024>, 4> LUTs = readLUTValues(input);
                input.close();

                std::ofstream output(evt.directory + "/temperatures.txt");
                logger->info("Temperatures... temperatures.txt");

                image::Image<uint16_t> im1 = cropIR(evt.images.image1);
                image::Image<uint16_t> im2 = cropIR(evt.images.image2);
                image::Image<uint16_t> im3 = cropIR(evt.images.image3);
                image::Image<uint16_t> im4 = cropIR(evt.images.image4);
                std::array<image::Image<uint16_t>, 4> channels = {im1, im2, im3, im4};

                geodetic::projection::GEOProjector proj(61.5, 35782.466981, 18990, 18956, 1.1737, 1.1753, 0, -40, 1);

                for (int j = 0; j < (int)points.size(); j++)
                {
                    int x, y;
                    proj.forward(points[j][1], points[j][0], x, y);
                    x /= 4;
                    y /= 4;

                    output << "Temperature measurements for point [" + std::to_string(x) + ", " + std::to_string(y) + "] (" + names[j] + ") with r = " + std::to_string(points[j][2]) << '\n'
                           << '\n';

                    logger->info("Temperature measurements for point [" + std::to_string(x) + ", " + std::to_string(y) + "] (" + names[j] + ") with r = " + std::to_string(points[j][2]));
                    for (int i = 0; i < 4; i++)
                    {
                        output << "    Channel " + std::to_string(i + 2) + ":     " << FIXED_FLOAT(LUTs[i][getAVG(channels[i], x, y, points[j][2]) >> 6])
                               << " K    (";
                        if (LUTs[i][getAVG(channels[i], x, y, points[j][2]) >> 6] - 273.15 < 10)
                        {
                            if (LUTs[i][getAVG(channels[i], x, y, points[j][2]) >> 6] - 273.15 <= -10)
                            {
                                output << FIXED_FLOAT(LUTs[i][getAVG(channels[i], x, y, points[j][2]) >> 6] - 273.15);
                            }
                            else if (LUTs[i][getAVG(channels[i], x, y, points[j][2]) >> 6] - 273.15 > 0)
                            {
                                output << "  " << FIXED_FLOAT(LUTs[i][getAVG(channels[i], x, y, points[j][2]) >> 6] - 273.15);
                            }
                            else
                            {
                                output << " " << FIXED_FLOAT(LUTs[i][getAVG(channels[i], x, y, points[j][2]) >> 6] - 273.15);
                            }
                        }
                        else
                        {
                            output << " " << FIXED_FLOAT(LUTs[i][getAVG(channels[i], x, y, points[j][2]) >> 6] - 273.15);
                        }
                        output << " °C)";
                        output << '\n';
                        logger->info("channel " + std::to_string(i + 2) + ":     " +
                                     std::to_string(LUTs[i][getAVG(channels[i], x, y, points[j][2]) >> 6]) +
                                     " K    (" + std::to_string(LUTs[i][getAVG(channels[i], x, y, points[j][2]) >> 6] - 273.15) + " °C)");
                    }
                    output << '\n'
                           << '\n';
                }
                output.close();
            }
            else
            {
                logger->warn("goes/gvar/goes" + std::to_string(evt.images.sat_number) + "_gvar_lut.txt LUT is missing! Temperature measurement will not be performed.");
            }
        }
        else
        {
            logger->info("Image is not a FD, temperature measurement will not be performed.");
        }

        logger->info("Generating mapped crops..");
        //mapped crops of europe. IR and VIS
        image::Image<uint16_t> mapProj = cropIR(evt.images.image3);
        drawMapOverlay(evt.images.sat_number, evt.timeUTC, mapProj);
        mapProj.crop(500, 50, 500 + 1560, 50 + 890);
        logger->info("Europe IR crop.. europe_IR.png");
        mapProj.to8bits().save_png(std::string(evt.directory + "/europe_IR.png").c_str());

        mapProj = cropVIS(evt.images.image5);
        drawMapOverlay(evt.images.sat_number, evt.timeUTC, mapProj);
        mapProj.crop(1348, 240, 1348 + 5928, 240 + 4120);
        logger->info("Europe VIS crop.. europe_VIS.png");
        mapProj.to8bits().save_png(std::string(evt.directory + "/europe_VIS.png").c_str());
        mapProj.clear();
    }

    static void gvarSaveFalceColorHandler(const goes::gvar::events::GVARSaveFCImageEvent &evt)
    {
        if (evt.sat_number == 13)
        {
            logger->info("Europe crop... europe.png");
            image::Image<uint8_t> crop = evt.false_color_image;
            if (crop.width() == 20836)
                crop.crop(3198, 240, 3198 + 5928, 240 + 4120);
            else
                crop.crop(1348, 240, 1348 + 5928, 240 + 4120);
            crop.save_png(std::string(evt.directory + "/europe.png").c_str());
        }
    }

    static std::array<std::array<float, 1024>, 4> readLUTValues(std::ifstream &LUT)
    {
        std::array<std::array<float, 1024>, 4> values;
        std::string tmp;
        //skip first 7 lines
        for (int i = 0; i < 7; i++)
        {
            std::getline(LUT, tmp);
        }
        //read LUTs
        for (int i = 0; i < 4; i++)
        {
            for (int j = 0; j < 1024; j++)
            {
                std::getline(LUT, tmp);
                values[i][j] = std::stof(tmp.substr(39, 7));
            }
            if (i != 3)
            {
                //skip det2 for first 3 channels (no det2 for ch6)
                for (int j = 0; j < 1030; j++)
                {
                    std::getline(LUT, tmp);
                }
            }
        }
        return values;
    }

    static unsigned short getAVG(image::Image<uint16_t> &image, int x, int y, int r)
    {
        uint64_t sum = 0;
        for (int i = 0; i < std::pow(r * 2 + 1, 2); i++)
        {
            sum += image[(y - r + i / (2 * r + 1))*image.width() + (x - r + i % (2 * r + 1))];
        }
        return sum / std::pow(r * 2 + 1, 2);
    }

    static image::Image<uint16_t> cropIR(image::Image<uint16_t> input)
    {
        image::Image<uint16_t> output = input;
        if (input.width() == 5206)
        {
            output.crop(0, 4749);
        }
        else if (input.width() == 5209)
        {
            output.crop(463, 5209);
        }
        else
        {
            logger->warn("Wrong IR image size (" + std::to_string(input.width()) + "), it will not be cropped");
        }
        return output;
    }

    static image::Image<uint16_t> cropVIS(image::Image<uint16_t> input)
    {
        image::Image<uint16_t> output = input;
        if (input.width() == 20824)
        {
            output.crop(0, 18990);
        }
        else if (input.width() == 20836)
        {
            output.crop(1852, 20826);
        }
        else
        {
            logger->warn("Wrong VIS image size (" + std::to_string(input.width()) + "), it will not be cropped");
        }
        return output;
    }

    static int getNORADFromSatNumber(int number)
    {
        if (number == 13)
            return 29155;
        else if (number == 14)
            return 35491;
        else if (number == 15)
            return 36411;
        else
            return 0;
    };

    static predict_position getSatellitePosition(int number, time_t time)
    {
        tle::TLE goes_tle = tle::getTLEfromNORAD(getNORADFromSatNumber(number));
        predict_orbital_elements_t *goes_object = predict_parse_tle(goes_tle.line1.c_str(), goes_tle.line2.c_str());
        predict_position goes_position;
        predict_orbit(goes_object, &goes_position, predict_to_julian(time));
        predict_destroy_orbital_elements(goes_object);
        return goes_position;
    }

    // Expect cropped IR
    static void drawMapOverlay(int number, time_t time, image::Image<uint16_t> &image)
    {
        geodetic::projection::GEOProjector proj(61.5, 35782.466981, 18990, 18956, 1.1737, 1.1753, 0, -40, 1);

        unsigned short color[3] = {65535, 65535, 65535};

        map::drawProjectedMapShapefile({resources::getResourcePath("maps/ne_10m_admin_0_countries.shp")}, image, color, [&proj, &image](float lat, float lon, int map_height, int map_width) -> std::pair<int, int>
                                       {
                                           int image_x, image_y;
                                           proj.forward(lon, lat, image_x, image_y);
                                           if (image.width() == 18990)
                                           {
                                               return {image_x, image_y};
                                           }
                                           else
                                           {
                                               if (image_x == -1 && image_y == -1)
                                               {
                                                   return {-1, -1};
                                               }
                                               else
                                               {
                                                   return {image_x / 4, image_y / 4};
                                               }
                                           }
                                       });

    }


public:
    std::string getID()
    {
        return "gvar_extended";
    }

    void init()
    {
        satdump::eventBus->register_handler<satdump::SatDumpStartedEvent>(satdumpStartedHandler);
        satdump::eventBus->register_handler<goes::gvar::events::GVARSaveChannelImagesEvent>(gvarSaveChannelImagesHandler);
        satdump::eventBus->register_handler<goes::gvar::events::GVARSaveFCImageEvent>(gvarSaveFalceColorHandler);
    }
};

std::string GVARExtended::misc_preview_text = "SatDump | GVAR";
std::vector<std::array<float, 3>> GVARExtended::points = {{}};
std::vector<std::string> GVARExtended::names = {};

PLUGIN_LOADER(GVARExtended)