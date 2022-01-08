#include "projection.h"

#include "imgui/imgui.h"
#include "global.h"
#include "logger.h"
#include "main_ui.h"
#include "imgui/imgui_image.h"

#include "common/image/image.h"

#include "imgui/file_selection.h"
#include "common/map/map_drawer.h"
#include "resources.h"
#include "common/geodetic/projection/stereo.h"
#include "common/geodetic/projection/tpers.h"
#include "common/map/maidenhead.h"
#include "common/geodetic/projection/proj_file.h"
#include "common/geodetic/projection/satellite_reprojector.h"
#include <filesystem>
#include "global.h"
#include "common/geodetic/projection/geo_projection.h"
#include "modules/goes/gvar/image/crop.h"
#include "settings.h"
#include "findgeoref.h"

namespace projection
{
    extern int output_width;
    extern int output_height;

    image::Image<uint8_t> projected_image;
    unsigned int textureID = 0;
    uint32_t *textureBuffer;

    // Settings
    int projection_type = 1;

    char stereo_locator[100];
    float stereo_scale = 1;

    float satel_alt = 30000;
    float satel_lon = 0;
    float satel_lat = 0;
    float satel_angle = 0;
    float satel_az = 0;

    bool draw_borders = true;
    bool draw_cities = true;
    bool draw_custom_labels = true;
    float cities_size_ratio = 0.32;

    char newfile_image[1000];
    char newfile_georef[1000];
    bool use_equirectangular = false;
    struct FileToProject
    {
        std::string filename;
        std::string timestamp;
        image::Image<uint8_t> image;
        std::shared_ptr<geodetic::projection::proj_file::GeodeticReferenceFile> georef;
        bool show;
        float opacity;

        float progress = 0;
    };
    std::vector<FileToProject> filesToProject;

    // Custom
    char new_custom_label_str[1000];
    float new_custom_label_lat = 0;
    float new_custom_label_lon = 0;
    bool show_custom_labels_window = false;
    std::vector<map::CustomLabel> customLabels;

    // Utils
    geodetic::projection::StereoProjection proj_stereo;
    geodetic::projection::TPERSProjection proj_satel;
    std::function<std::pair<int, int>(float, float, int, int)> projectionFunc;

    bool isFirstUiRun = false;
    bool rendering = false;
    int isRenderDone = false;

    float render_progress = 0;

    void initProjection()
    {
        projected_image = image::Image<uint8_t>(output_width, output_height, 3);

        // Init texture
        textureID = makeImageTexture();
        textureBuffer = new uint32_t[output_width * output_height];
        uchar_to_rgba(projected_image.data(), textureBuffer, output_width * output_height, 3);
        updateImageTexture(textureID, textureBuffer, output_width, output_height);

        // Init settings
        std::fill(stereo_locator, &stereo_locator[100], 0);

        std::fill(newfile_image, &newfile_image[1000], 0);
        std::fill(newfile_georef, &newfile_georef[1000], 0);

        std::fill(new_custom_label_str, &new_custom_label_str[1000], 0);

        isFirstUiRun = true;
        rendering = false;

        // Load custom labels
        customLabels.clear();
        if (settings.count("custom_map_labels") > 0)
        {
            std::vector<std::tuple<std::string, double, double>> labelTuple = settings["custom_map_labels"].get<std::vector<std::tuple<std::string, double, double>>>();
            for (std::tuple<std::string, double, double> &label : labelTuple)
                customLabels.push_back({std::get<0>(label), std::get<1>(label), std::get<2>(label)});
        }
    }

    void destroyProjection()
    {
        filesToProject.clear();
        deleteImageTexture(textureID);
        delete[] textureBuffer;
        projected_image = image::Image<uint8_t>(1, 1, 3);
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
        else if (projection_type == 2) // Satellite
        {
            logger->info("Projecting to satellite point of view at " + std::to_string(satel_lon) + " longitude and " + std::to_string(satel_lat) + " latitude.");
            proj_satel.init(satel_alt * 1000, satel_lon, satel_lat, satel_angle, satel_az);
            projectionFunc = [](float lat, float lon, int map_height, int map_width) -> std::pair<int, int>
            {
                double x, y;
                proj_satel.forward(lon, lat, x, y);
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
                geodetic::projection::projectEQUIToproj(toProj.image, projected_image, toProj.image.channels(), projectionFunc, toProj.opacity, (float *)&toProj.progress);
            }
            else if (toProj.georef->file_type == geodetic::projection::proj_file::LEO_TYPE)
            {
                geodetic::projection::proj_file::LEO_GeodeticReferenceFile leofile = *((geodetic::projection::proj_file::LEO_GeodeticReferenceFile *)toProj.georef.get());
                std::shared_ptr<geodetic::projection::LEOScanProjectorSettings> settings = leoProjectionRefFile(leofile);
                geodetic::projection::LEOScanProjector projector(settings);
                logger->info("Reprojecting LEO...");
                geodetic::projection::reprojectLEOtoProj(toProj.image, projector, projected_image, toProj.image.channels(), projectionFunc, toProj.opacity, (float *)&toProj.progress);
            }
            else if (toProj.georef->file_type == geodetic::projection::proj_file::GEO_TYPE)
            {
                geodetic::projection::proj_file::GEO_GeodeticReferenceFile gsofile = *((geodetic::projection::proj_file::GEO_GeodeticReferenceFile *)toProj.georef.get());
                logger->info("Reprojecting GEO...");
                geodetic::projection::GEOProjector projector = geodetic::projection::proj_file::geoProjectionRefFile(gsofile);
                geodetic::projection::reprojectGEOtoProj(toProj.image, projector, projected_image, toProj.image.channels(), projectionFunc, toProj.opacity, (float *)&toProj.progress);
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

        // Should we draw custom labels?
        if (draw_custom_labels)
        {
            unsigned char color[3] = {0, 0, 255};
            map::drawProjectedLabels(customLabels, projected_image, color, projectionFunc, cities_size_ratio);
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

            if (isRenderDone > 0)
            {
                isRenderDone--;
            }
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
            ImGui::RadioButton("Satellite View", &projection_type, 2);

            if (projection_type == 0) // Equi
            {
            }
            else if (projection_type == 1) // Stereographic
            {
                ImGui::InputText("Center Locator", stereo_locator, 100);
                ImGui::InputFloat("Projection Scale", &stereo_scale, 0.1, 1);
            }
            else if (projection_type == 2) // Satellite
            {
                ImGui::InputFloat("Satellite Altitude", &satel_alt, 0.1, 10);
                ImGui::InputFloat("Satellite Longitude", &satel_lon, 0.1, 10);
                ImGui::InputFloat("Satellite Latitude", &satel_lat, 0.1, 10);
                ImGui::InputFloat("Satellite Tilt", &satel_angle, 0.1, 10);
                ImGui::InputFloat("Satellite Azimuth", &satel_az, 0.1, 10);
            }

            ImGui::Checkbox("Draw Borders", &draw_borders);
            ImGui::Checkbox("Draw Cities", &draw_cities);
            ImGui::Checkbox("Draw Labels", &draw_custom_labels);
            ImGui::InputFloat("Cities Scale", &cities_size_ratio);
            if (ImGui::Button("Edit Custom Labels"))
                show_custom_labels_window = true;

            if (!rendering)
            {
                if (ImGui::Button("Render"))
                {
                    if (!rendering)
                        processThreadPool.push([&](int)
                                               {
                                                   doRender();
                                                   rendering = false;
                                                   isRenderDone = 10;
                                               });
                    rendering = true;
                    isRenderDone = false;
                }
            }
            else
                ImGui::Button("Rendering...");

            ImGui::SameLine();

            if (ImGui::Button("Save"))
            {
#ifdef __APPLE__
                projected_image.save_png("projection.png");
#else
                logger->debug("Opening file dialog");
                std::string output_file = selectOutputFileDialog("Save projection to", "projection.png", {"*.png"});

                logger->info("Saving to " + output_file);

                if (output_file.size() > 0)
                {
                    projected_image.save_png(output_file.c_str());
                }
#endif
            }

            ImGui::SameLine();

            if (ImGui::Button("Exit"))
            {
                destroyProjection();
            }

            // Calculate progress, if rendering
            if (rendering)
            {
                render_progress = 0;
                for (const FileToProject &toProj : filesToProject)
                    render_progress += toProj.progress;
                render_progress /= filesToProject.size();
            }
            else
            {
                render_progress = 0;
            }

            ImGui::SameLine();
            ImGui::ProgressBar(render_progress);
        }
        ImGui::End();

        if (show_custom_labels_window)
        {
            ImGui::Begin(std::string("Edit Custom Labels").c_str(), &show_custom_labels_window);
            {
                if (ImGui::BeginTable("CustomlabelTable", 4, ImGuiTableFlags_Borders))
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Label");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("Lat");
                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("Lon");
                    ImGui::TableSetColumnIndex(3);
                    for (map::CustomLabel &label : customLabels)
                    {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("%s", label.label.c_str());
                        ImGui::TableSetColumnIndex(1);
                        ImGui::Text("%f", label.lat);
                        ImGui::TableSetColumnIndex(2);
                        ImGui::Text("%f", label.lon);
                        ImGui::TableSetColumnIndex(3);
                        if (ImGui::Button(std::string("Delete##" + label.label).c_str()))
                        {
                            logger->warn("Deleting " + label.label);
                            customLabels.erase(std::find_if(customLabels.begin(), customLabels.end(), [&label](map::CustomLabel &l)
                                                            { return l.label == label.label; }));
                            break;
                        }
                    }
                    ImGui::EndTable();

                    ImGui::InputText("Label Name", new_custom_label_str, 1000);
                    ImGui::InputFloat("Label Latitude", &new_custom_label_lat, 0.1, 10);
                    ImGui::InputFloat("Label Longitude", &new_custom_label_lon, 0.1, 10);
                    if (ImGui::Button("Add##addcustomlabel"))
                    {
                        customLabels.push_back({std::string(new_custom_label_str), new_custom_label_lat, new_custom_label_lon});
                        std::fill(new_custom_label_str, &new_custom_label_str[1000], 0);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Save"))
                    {
                        std::vector<std::tuple<std::string, double, double>> labelTuple;
                        for (map::CustomLabel &label : customLabels)
                        {
                            labelTuple.push_back({label.label, label.lat, label.lon});
                        }
                        settings["custom_map_labels"] = labelTuple;
                        saveSettings();
                    }
                }
            }
            ImGui::End();
        }

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
                    if (ImGui::Button(std::string("Delete##" + toProj.timestamp).c_str()))
                    {
                        logger->warn("Deleting " + toProj.filename);
                        filesToProject.erase(std::find_if(filesToProject.begin(), filesToProject.end(), [&toProj](FileToProject &file)
                                                          { return toProj.filename == file.filename && toProj.timestamp == file.timestamp; }));
                        logger->info(filesToProject.size());
                        break;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(std::string("View##" + toProj.timestamp).c_str()))
                        toProj.show = true;
                }
                ImGui::EndTable();
            }

            ImGui::InputText("New Image ", newfile_image, 1000);
            if (ImGui::Button("Select Image"))
            {
                logger->debug("Opening file dialog");

                std::string input_file = selectInputFileDialog("Open input image", ".", {".*"});

                if (input_file.size() > 0)
                {
                    logger->debug("Dir " + input_file);
                    std::fill(newfile_image, &newfile_image[1000], 0);
                    std::memcpy(newfile_image, input_file.c_str(), input_file.length());

                    std::optional<std::string> georef = findGeoRefForImage(input_file);
                    if (georef.has_value())
                    {
                        std::fill(newfile_georef, &newfile_georef[1000], 0);
                        std::memcpy(newfile_georef, georef.value().c_str(), georef.value().length());
                    }
                }
            }

            if (!use_equirectangular)
            {
                ImGui::InputText("New Georef", newfile_georef, 1000);
                if (ImGui::Button("Select Georef"))
                {
                    logger->debug("Opening file dialog");
                    std::string input_file = selectInputFileDialog("Open input georef", ".", {".georef"});

                    if (input_file.size() > 0)
                    {
                        logger->debug("Dir " + input_file);
                        std::fill(newfile_georef, &newfile_georef[1000], 0);
                        std::memcpy(newfile_georef, input_file.c_str(), input_file.length());
                    }
                }
            }

            ImGui::Checkbox("Equirectangular input", &use_equirectangular);

            if (ImGui::Button("Add file") && std::filesystem::exists(newfile_image) && (std::filesystem::exists(newfile_georef) || use_equirectangular))
            {
                //unsigned int bit_depth;
                //cimg_library::CImg<unsigned short> new_image_full16;
                //new_image_full16.load_png(newfile_image, &bit_depth);
                image::Image<uint8_t> new_image; // = new_image_full16 >> (bit_depth == 16 ? 8 : 0);
                new_image.load_png(newfile_image);
                //new_image_full16.clear(); // Free up memory now

                std::shared_ptr<geodetic::projection::proj_file::GeodeticReferenceFile> new_geofile;
                if (use_equirectangular)
                {
                    new_geofile = std::make_shared<geodetic::projection::proj_file::GeodeticReferenceFile>();
                    new_geofile->file_type = 0;
                    new_geofile->utc_timestamp_seconds = 0;
                }
                else
                {
                    new_geofile = geodetic::projection::proj_file::readReferenceFile(std::string(newfile_georef));
                }

                if (new_geofile->file_type == geodetic::projection::proj_file::GEO_TYPE)
                {
                    geodetic::projection::proj_file::GEO_GeodeticReferenceFile gsofile = *((geodetic::projection::proj_file::GEO_GeodeticReferenceFile *)new_geofile.get());

                    // If this is GOES-N Data, we need to crop it first!
                    if (gsofile.norad == 29155 || gsofile.norad == 35491 || gsofile.norad == 36441)
                    {
                        logger->info("Cropping GOES-N data...");

                        // One of those should work
                        new_image = goes::gvar::cropIR(new_image);  // IR Case
                        new_image = goes::gvar::cropVIS(new_image); // VIS Case
                    }

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
                                          false,
                                          1.0});

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
                    if (ImGui::BeginTabBar(std::string("##tabbar" + toProj.timestamp).c_str()))
                    {
                        if (ImGui::BeginTabItem("Contens"))
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
                                else if (toProj.georef->file_type == 2)
                                    ImGui::Text("2, LEO");
                                else
                                    ImGui::Text("Invalid");

                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);
                                ImGui::Text("Timestamp");
                                ImGui::TableSetColumnIndex(1);
                                ImGui::Text("%s", toProj.timestamp.c_str());

                                if (toProj.georef->file_type == geodetic::projection::proj_file::GEO_TYPE)
                                {
                                    geodetic::projection::proj_file::GEO_GeodeticReferenceFile &gsofile = *((geodetic::projection::proj_file::GEO_GeodeticReferenceFile *)toProj.georef.get());

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
                                else if (toProj.georef->file_type == geodetic::projection::proj_file::LEO_TYPE)
                                {
                                    geodetic::projection::proj_file::LEO_GeodeticReferenceFile &leofile = *((geodetic::projection::proj_file::LEO_GeodeticReferenceFile *)toProj.georef.get());

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
                                    ImGui::Text("Scan Angle");
                                    ImGui::TableSetColumnIndex(1);
                                    ImGui::InputDouble("##scan_angle", &leofile.scanline.scan_angle);

                                    ImGui::TableNextRow();
                                    ImGui::TableSetColumnIndex(0);
                                    ImGui::Text("Roll Offset");
                                    ImGui::TableSetColumnIndex(1);
                                    ImGui::InputDouble("##roll_offset", &leofile.scanline.roll_offset);

                                    ImGui::TableNextRow();
                                    ImGui::TableSetColumnIndex(0);
                                    ImGui::Text("Pitch Offset");
                                    ImGui::TableSetColumnIndex(1);
                                    ImGui::InputDouble("##pitch_offset", &leofile.scanline.pitch_offset);

                                    ImGui::TableNextRow();
                                    ImGui::TableSetColumnIndex(0);
                                    ImGui::Text("Yaw Offset");
                                    ImGui::TableSetColumnIndex(1);
                                    ImGui::InputDouble("##yaw_offset", &leofile.scanline.yaw_offset);

                                    ImGui::TableNextRow();
                                    ImGui::TableSetColumnIndex(0);
                                    ImGui::Text("Time offset");
                                    ImGui::TableSetColumnIndex(1);
                                    ImGui::InputDouble("##time_offset", &leofile.scanline.time_offset);

                                    ImGui::TableNextRow();
                                    ImGui::TableSetColumnIndex(0);
                                    ImGui::Text("Image width");
                                    ImGui::TableSetColumnIndex(1);
                                    ImGui::Text("%d", leofile.scanline.image_width);

                                    ImGui::TableNextRow();
                                    ImGui::TableSetColumnIndex(0);
                                    ImGui::Text("Invert scan");
                                    ImGui::TableSetColumnIndex(1);
                                    ImGui::Text("%d", leofile.scanline.invert_scan);

                                    ImGui::TableNextRow();
                                    ImGui::TableSetColumnIndex(0);
                                    ImGui::Text("Timestamp count");
                                    ImGui::TableSetColumnIndex(1);
                                    ImGui::Text("%llu", leofile.scanline.timestamp_count);
                                }

                                ImGui::EndTable();
                            }

                            ImGui::EndTabItem();
                        }
                        if (ImGui::BeginTabItem("Settings"))
                        {
                            ImGui::InputFloat(std::string("##opacity" + toProj.timestamp).c_str(), &toProj.opacity);
                            ImGui::SameLine();
                            ImGui::Text("Opacity");

                            ImGui::EndTabItem();
                        }
                        ImGui::EndTabBar();
                    }
                }
                ImGui::End();
            }
        }

        if (isFirstUiRun)
            isFirstUiRun = false;
    }
};
