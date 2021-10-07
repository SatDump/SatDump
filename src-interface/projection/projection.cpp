#include "projection.h"

#include "imgui/imgui.h"
#include "global.h"
#include "logger.h"
#include "main_ui.h"
#include "imgui/imgui_image.h"

#define cimg_use_png
#define cimg_display 0
#include "CImg.h"

#ifndef __ANDROID__
#include "portable-file-dialogs.h"
#endif
#ifdef __ANDROID__
std::string getFilePath();
std::string getDirPath();
#endif

#include "common/map/map_drawer.h"
#include "resources.h"
#include "common/projection/stereo.h"
#include "common/projection/geos.h"
#include "common/map/maidenhead.h"
#include "common/projection/proj_file.h"
#include "common/projection/satellite_reprojector.h"
#include <filesystem>
#include "global.h"
#include "common/projection/geo_projection.h"

namespace projection
{
    extern int output_width;
    extern int output_height;

    cimg_library::CImg<unsigned char> projected_image;
    unsigned int textureID = 0;
    uint32_t *textureBuffer;

    // Settings
    int projection_type = 1;
    char stereo_locator[100];
    float stereo_scale = 1;
    float geos_lon = 0;
    bool draw_borders = true;
    bool draw_cities = true;
    float cities_size_ratio = 0.3;

    char newfile_image[1000];
    char newfile_georef[1000];
    bool use_equirectangular = false;
    struct FileToProject
    {
        std::string filename;
        std::string timestamp;
        cimg_library::CImg<unsigned short> image;
        std::shared_ptr<projection::proj_file::GeodeticReferenceFile> georef;
        bool show;
    };
    std::vector<FileToProject> filesToProject;

    // Utils
    projection::StereoProjection proj_stereo;
    projection::GEOSProjection proj_geos;
    std::function<std::pair<int, int>(float, float, int, int)> projectionFunc;

    bool isFirstUiRun = false;
    bool rendering = false;
    bool isRenderDone = false;

    void initProjection()
    {
        projected_image = cimg_library::CImg<unsigned char>(output_width, output_height, 1, 3, 0);

        // Init texture
        textureID = makeImageTexture();
        textureBuffer = new uint32_t[output_width * output_height];
        uchar_to_rgba(projected_image.data(), textureBuffer, output_width * output_height, 3);
        updateImageTexture(textureID, textureBuffer, output_width, output_height);

        // Init settings
        std::fill(stereo_locator, &stereo_locator[100], 0);

        std::fill(newfile_image, &newfile_image[1000], 0);
        std::fill(newfile_georef, &newfile_georef[1000], 0);

        isFirstUiRun = true;
        rendering = false;
    }

    void destroyProjection()
    {
        filesToProject.clear();
        deleteImageTexture(textureID);
        delete[] textureBuffer;
        projected_image = cimg_library::CImg<unsigned char>(1, 1, 1, 3, 0);
        satdumpUiStatus = MAIN_MENU;
    }

    void doRender()
    {
        // Clean image
        projected_image.fill(0);

        // Setup proj
        if (projection_type == 0) // Equi
        {
            projectionFunc = [](float lat, float lon, int map_height, int map_width) -> std::pair<int, int>
            {
                int imageLat = map_height - ((90.0f + lat) / 180.0f) * map_height;
                int imageLon = (lon / 360.0f) * map_width + (map_width / 2);
                return {imageLon, imageLat};
            };
        }
        else if (projection_type == 1) // Stereographic
        {
            logger->info("Projecting to Stereographic centered around " + std::string(stereo_locator));
            proj_stereo.init(mh2lat((char *)stereo_locator), mh2lon((char *)stereo_locator));
            projectionFunc = [](float lat, float lon, int map_height, int map_width) -> std::pair<int, int>
            {
                double x, y;
                proj_stereo.forward(lon, lat, x, y);
                x *= map_width / stereo_scale;
                y *= map_height / stereo_scale;
                return {x + (map_width / 2), map_height - (y + (map_height / 2))};
            };
        }
        else if (projection_type == 2) // GEOS
        {
            logger->info("Projecting to Geostationnary point of view at " + std::to_string(geos_lon) + " longitude.");
            proj_geos.init(30000000, geos_lon);
            projectionFunc = [](float lat, float lon, int map_height, int map_width) -> std::pair<int, int>
            {
                double x, y;
                proj_geos.forward(lon, lat, x, y);
                x *= map_width / 2;
                y *= map_height / 2;
                return {x + (map_width / 2), map_height - (y + (map_height / 2))};
            };
        }

        // Project files
        for (const FileToProject &toProj : filesToProject)
        {
            if (toProj.georef->file_type == 0)
            {
                logger->info("Reprojecting Equiectangular...");
                projection::projectEQUIToproj(toProj.image, projected_image, toProj.image.spectrum(), projectionFunc);
            }
            else if (toProj.georef->file_type == projection::proj_file::LEO_TYPE)
            {
                projection::proj_file::LEO_GeodeticReferenceFile leofile = *((projection::proj_file::LEO_GeodeticReferenceFile *)toProj.georef.get());
                projection::LEOScanProjectorSettings settings = leoProjectionRefFile(leofile);
                projection::LEOScanProjector projector(settings);
                logger->info("Reprojecting LEO...");
                projection::reprojectLEOtoProj(toProj.image, projector, projected_image, toProj.image.spectrum(), projectionFunc);
            }
            else if (toProj.georef->file_type == projection::proj_file::GEO_TYPE)
            {
                projection::proj_file::GEO_GeodeticReferenceFile gsofile = *((projection::proj_file::GEO_GeodeticReferenceFile *)toProj.georef.get());
                logger->info("Reprojecting GEO...");
                projection::GEOProjector projector = projection::proj_file::geoProjectionRefFile(gsofile);
                projection::reprojectGEOtoProj(toProj.image, projector, projected_image, toProj.image.spectrum(), projectionFunc);
            }
        }

        // Should we draw borders?
        if (draw_borders)
        {
            unsigned char color[3] = {0, 255, 0};
            map::drawProjectedMapShapefile({resources::getResourcePath("maps/ne_10m_admin_0_countries.shp")}, projected_image, color, projectionFunc);
        }

        // Should we draw cities?
        if (draw_cities)
        {
            unsigned char color[3] = {255, 0, 0};
            map::drawProjectedCapitalsGeoJson({resources::getResourcePath("maps/ne_10m_populated_places_simple.json")}, projected_image, color, projectionFunc, cities_size_ratio);
        }

        // Finally, update image
        //uchar_to_rgba(projected_image.data(), textureBuffer, output_width * output_height, 3);
        //updateImageTexture(textureID, textureBuffer, output_width, output_height);
    };

    void renderProjection(int wwidth, int wheight)
    {
        if (rendering || isRenderDone)
        {
            // When rendering, update more often
            uchar_to_rgba(projected_image.data(), textureBuffer, output_width * output_height, 3);
            updateImageTexture(textureID, textureBuffer, output_width, output_height);

            if (isRenderDone)
                isRenderDone = false;
        }

        ImGui::SetNextWindowPos({0, 0});
        ImGui::SetNextWindowSize({(float)wwidth, (float)wheight});
        ImGui::Begin("Projection Image", NULL, NOWINDOW_FLAGS | ImGuiWindowFlags_NoTitleBar);
        {
            float ratioX = ImGui::GetWindowWidth() / output_width;
            float ratioY = (ImGui::GetWindowHeight() - 16) / output_height;
            float ratio = ratioX > ratioY ? ratioY : ratioX;
            ImGui::SetCursorPos({(ImGui::GetWindowWidth() / 2) - (output_width * ratio) / 2, (ImGui::GetWindowHeight() / 2) - (output_height * ratio) / 2});
            ImGui::Image((void *)(intptr_t)textureID, {output_width * ratio, output_height * ratio});
        }
        ImGui::End();

        if (isFirstUiRun)
            ImGui::SetNextWindowSize({400 * ui_scale, 250 * ui_scale});
        ImGui::Begin("Projection");
        {
            ImGui::RadioButton("Equirectangular", &projection_type, 0);
            ImGui::SameLine();
            ImGui::RadioButton("Stereographic", &projection_type, 1);
            ImGui::SameLine();
            ImGui::RadioButton("Geostationnary", &projection_type, 2);

            if (projection_type == 0) // Equi
            {
            }
            else if (projection_type == 1) // Stereographic
            {
                ImGui::InputText("Center Locator", stereo_locator, 100);
                ImGui::InputFloat("Projection Scale", &stereo_scale, 0.1, 1);
            }
            else if (projection_type == 2) // GEOS
            {
                ImGui::InputFloat("Center Longitude", &geos_lon, 0.1, 10);
            }

            ImGui::Checkbox("Draw Borders", &draw_borders);
            ImGui::Checkbox("Draw Cities", &draw_cities);
            ImGui::InputFloat("Cities Scale", &cities_size_ratio);

            if (!rendering)
            {
                if (ImGui::Button("Render"))
                {
                    if (!rendering)
                        processThreadPool.push([&](int)
                                               {
                                                   doRender();
                                                   rendering = false;
                                               });
                    rendering = true;
                    isRenderDone = true;
                }
            }
            else
                ImGui::Button("Rendering...");

            ImGui::SameLine();

            if (ImGui::Button("Save"))
            {
                logger->info("Saving to projection.png");
                projected_image.save_png("projection.png");
            }

            ImGui::SameLine();

            if (ImGui::Button("Exit"))
            {
                destroyProjection();
            }
        }
        ImGui::End();

        if (isFirstUiRun)
            ImGui::SetNextWindowSize({300 * ui_scale, 400 * ui_scale});
        ImGui::Begin("File Manager");
        {
            if (ImGui::BeginTable("LoadedFiles", 3, ImGuiTableFlags_Borders))
            {
                for (FileToProject &toProj : filesToProject)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%s", toProj.filename.c_str());
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%s", toProj.timestamp.c_str());
                    ImGui::TableSetColumnIndex(2);
                    if (ImGui::Button("Delete"))
                    {
                        logger->warn("Deleting " + toProj.filename);
                        filesToProject.erase(std::find_if(filesToProject.begin(), filesToProject.end(), [&toProj](FileToProject &file)
                                                          { return toProj.filename == file.filename && toProj.timestamp == file.timestamp; }));
                        logger->info(filesToProject.size());
                        break;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("View"))
                    {
                        toProj.show = true;
                    }
                }

                ImGui::EndTable();
            }

            ImGui::InputText("New Image ", newfile_image, 1000);
            if (ImGui::Button("Select Image"))
            {

                logger->debug("Opening file dialog");
                std::string input_file;
#ifdef __ANDROID__
                input_file = getFilePath();
#else
                auto result = pfd::open_file("Open input image", ".", {".*"}, pfd::opt::none);
                while (result.ready(1000))
                {
                }

                if (result.result().size() > 0)
                    input_file = result.result()[0];
#endif
                logger->debug("Dir " + input_file);
                std::fill(newfile_image, &newfile_image[1000], 0);
                std::memcpy(newfile_image, input_file.c_str(), input_file.length());
            }

            if (!use_equirectangular)
            {
                ImGui::InputText("New Georef", newfile_georef, 1000);
                if (ImGui::Button("Select Georef"))
                {

                    logger->debug("Opening file dialog");
                    std::string input_file;
#ifdef __ANDROID__
                    input_file = getFilePath();
#else
                    auto result = pfd::open_file("Open input georef", ".", {".georef"}, pfd::opt::none);
                    while (result.ready(1000))
                    {
                    }

                    if (result.result().size() > 0)
                        input_file = result.result()[0];
#endif
                    logger->debug("Dir " + input_file);
                    std::fill(newfile_georef, &newfile_georef[1000], 0);
                    std::memcpy(newfile_georef, input_file.c_str(), input_file.length());
                }
            }

            ImGui::Checkbox("Equirectangular input", &use_equirectangular);

            if (ImGui::Button("Add file") && std::filesystem::exists(newfile_image) && (std::filesystem::exists(newfile_georef) || use_equirectangular))
            {
                cimg_library::CImg<unsigned short> new_image(newfile_image);
                new_image.normalize(0, 65535);

                std::shared_ptr<projection::proj_file::GeodeticReferenceFile> new_geofile;
                if (use_equirectangular)
                {
                    new_geofile = std::make_shared<projection::proj_file::GeodeticReferenceFile>();
                    new_geofile->file_type = 0;
                    new_geofile->utc_timestamp_seconds = 0;
                }
                else
                {
                    new_geofile = projection::proj_file::readReferenceFile(std::string(newfile_georef));
                }

                if (new_geofile->file_type == projection::proj_file::GEO_TYPE)
                {
                    projection::proj_file::GEO_GeodeticReferenceFile gsofile = *((projection::proj_file::GEO_GeodeticReferenceFile *)new_geofile.get());
                    new_image.resize(gsofile.image_width, gsofile.image_height); // Safety
                }

                time_t secondsTime = new_geofile->utc_timestamp_seconds;
                std::tm *timeReadable = gmtime(&secondsTime);
                std::string timestamp = std::to_string(timeReadable->tm_year + 1900) + "/" +
                                        (timeReadable->tm_mon + 1 > 9 ? std::to_string(timeReadable->tm_mon + 1) : "0" + std::to_string(timeReadable->tm_mon + 1)) + "/" +
                                        (timeReadable->tm_mday > 9 ? std::to_string(timeReadable->tm_mday) : "0" + std::to_string(timeReadable->tm_mday)) + " " +
                                        (timeReadable->tm_hour > 9 ? std::to_string(timeReadable->tm_hour) : "0" + std::to_string(timeReadable->tm_hour)) + ":" +
                                        (timeReadable->tm_min > 9 ? std::to_string(timeReadable->tm_min) : "0" + std::to_string(timeReadable->tm_min)) + ":" +
                                        (timeReadable->tm_sec > 9 ? std::to_string(timeReadable->tm_sec) : "0" + std::to_string(timeReadable->tm_sec));

                filesToProject.push_back({std::string(newfile_image).substr(std::string(newfile_image).find_last_of("/\\") + 1),
                                          timestamp + " UTC",
                                          new_image,
                                          new_geofile,
                                          false});

                std::fill(newfile_image, &newfile_image[100], 0);
                std::fill(newfile_georef, &newfile_georef[100], 0);
            }

            ImGui::Text("For about all images, you will need to input \n"
                        "both the image itself and the .georef file, \n"
                        "usually in the same output folder. \n"
                        "Uncorrected images should be used. \n"
                        "The equirectangular option is used for \n"
                        "inputing external data such as NASA's \n"
                        "blue marble.");
        }
        ImGui::End();

        for (FileToProject &toProj : filesToProject)
        {
            if (toProj.show)
            {
                ImGui::Begin(std::string("View georef for " + std::string(toProj.filename)).c_str(), &toProj.show);
                {
                    if (ImGui::BeginTable("FileContentsLEO", 2, ImGuiTableFlags_Borders))
                    {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("File Type");
                        ImGui::TableSetColumnIndex(1);
                        if (toProj.georef->file_type == 0)
                            ImGui::Text("0, Empty");
                        else if (toProj.georef->file_type == 1)
                            ImGui::Text("1, GEO");
                        else if (toProj.georef->file_type == 1)
                            ImGui::Text("2, LEO");
                        else
                            ImGui::Text("Invalid");

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Timestamp");
                        ImGui::TableSetColumnIndex(1);
                        ImGui::Text("%s", toProj.timestamp.c_str());

                        if (toProj.georef->file_type == projection::proj_file::GEO_TYPE)
                        {
                            projection::proj_file::GEO_GeodeticReferenceFile gsofile = *((projection::proj_file::GEO_GeodeticReferenceFile *)toProj.georef.get());

                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("NORAD");
                            ImGui::TableSetColumnIndex(1);
                            ImGui::Text("%d", gsofile.norad);

                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("Satellite Longitude");
                            ImGui::TableSetColumnIndex(1);
                            ImGui::Text("%f", gsofile.position_longitude);

                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("Satellite Altitude");
                            ImGui::TableSetColumnIndex(1);
                            ImGui::Text("%f", gsofile.position_height);

                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("Projection Type");
                            ImGui::TableSetColumnIndex(1);
                            if (gsofile.projection_type == 0)
                                ImGui::Text("0, Full Disk");
                            else
                                ImGui::Text("Invalid");

                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("Image Width");
                            ImGui::TableSetColumnIndex(1);
                            ImGui::Text("%d", gsofile.image_width);

                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("Image Height");
                            ImGui::TableSetColumnIndex(1);
                            ImGui::Text("%d", gsofile.image_height);

                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("Horizontal Scale");
                            ImGui::TableSetColumnIndex(1);
                            ImGui::Text("%f", gsofile.horizontal_scale);

                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("Vertical Scale");
                            ImGui::TableSetColumnIndex(1);
                            ImGui::Text("%f", gsofile.vertical_scale);

                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("Horizontal Offset");
                            ImGui::TableSetColumnIndex(1);
                            ImGui::Text("%f", gsofile.horizontal_offset);

                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("Vertical Offset");
                            ImGui::TableSetColumnIndex(1);
                            ImGui::Text("%f", gsofile.vertical_offset);

                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("Sweep X");
                            ImGui::TableSetColumnIndex(1);
                            ImGui::Text("%d", gsofile.proj_sweep_x);
                        }
                        else if (toProj.georef->file_type == projection::proj_file::LEO_TYPE)
                        {
                            projection::proj_file::LEO_GeodeticReferenceFile leofile = *((projection::proj_file::LEO_GeodeticReferenceFile *)toProj.georef.get());

                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("NORAD");
                            ImGui::TableSetColumnIndex(1);
                            ImGui::Text("%d", leofile.norad);

                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("TLE 1");
                            ImGui::TableSetColumnIndex(1);
                            ImGui::Text("%s", leofile.tle_line1_data.c_str());

                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("TLE 2");
                            ImGui::TableSetColumnIndex(1);
                            ImGui::Text("%s", leofile.tle_line2_data.c_str());

                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("Projection Type");
                            ImGui::TableSetColumnIndex(1);
                            if (leofile.projection_type == 0)
                                ImGui::Text("0, Single Scanline");
                            else
                                ImGui::Text("Invalid");

                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("Correction Swath");
                            ImGui::TableSetColumnIndex(1);
                            ImGui::Text("%d", leofile.correction_swath);

                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("Correction Res");
                            ImGui::TableSetColumnIndex(1);
                            ImGui::Text("%f", leofile.correction_res);

                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("Correction Height");
                            ImGui::TableSetColumnIndex(1);
                            ImGui::Text("%f", leofile.correction_height);

                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("Instrument Swath");
                            ImGui::TableSetColumnIndex(1);
                            ImGui::Text("%f", leofile.instrument_swath);

                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("Projection Scale");
                            ImGui::TableSetColumnIndex(1);
                            ImGui::Text("%f", leofile.proj_scale);

                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("Azimuth Offset");
                            ImGui::TableSetColumnIndex(1);
                            ImGui::Text("%f", leofile.az_offset);

                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("Tilt offset");
                            ImGui::TableSetColumnIndex(1);
                            ImGui::Text("%f", leofile.tilt_offset);

                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("Time offset");
                            ImGui::TableSetColumnIndex(1);
                            ImGui::Text("%f", leofile.time_offset);

                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("Image width");
                            ImGui::TableSetColumnIndex(1);
                            ImGui::Text("%d", leofile.image_width);

                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("Invert scan");
                            ImGui::TableSetColumnIndex(1);
                            ImGui::Text("%d", leofile.invert_scan);

                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("Timestamp count");
                            ImGui::TableSetColumnIndex(1);
                            ImGui::Text("%d", leofile.timestamp_count);
                        }

                        ImGui::EndTable();
                    }
                }
                ImGui::End();
            }
        }

        if (isFirstUiRun)
            isFirstUiRun = false;
    }
};