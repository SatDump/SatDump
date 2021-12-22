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
    extern char overlay_image[1000];
    extern char overlay_georef[1000];
}

namespace projection_overlay
{
    image::Image<uint8_t> overlayed_image;
    unsigned int textureID = 0;
    uint32_t *textureBuffer;

    std::mutex imageMutex;
    std::shared_ptr<geodetic::projection::proj_file::GeodeticReferenceFile> geofile;

    bool invert_image = false;

    bool draw_borders = true;
    bool draw_cities = true;
    bool draw_custom_labels = true;
    float cities_size_ratio = 0.3;

    // Custom
    char new_custom_label_str[1000];
    float new_custom_label_lat = 0;
    float new_custom_label_lon = 0;
    bool show_custom_labels_window = false;
    std::vector<map::CustomLabel> customLabels;

    bool isFirstUiRun = false;
    bool rendering = false;
    int isRenderDone = false;

    void loadImage()
    {
        imageMutex.lock();
        overlayed_image.load_png(projection::overlay_image);

        if (geofile->file_type == geodetic::projection::proj_file::GEO_TYPE)
        {
            geodetic::projection::proj_file::GEO_GeodeticReferenceFile gsofile = *((geodetic::projection::proj_file::GEO_GeodeticReferenceFile *)geofile.get());

            // If this is GOES-N Data, we need to crop it first!
            if (gsofile.norad == 29155 || gsofile.norad == 35491 || gsofile.norad == 36441)
            {
                logger->info("Cropping GOES-N data...");

                // One of those should work
                overlayed_image = goes::gvar::cropIR(overlayed_image);  // IR Case
                overlayed_image = goes::gvar::cropVIS(overlayed_image); // VIS Case
            }

            overlayed_image.resize(gsofile.image_width, gsofile.image_height); // Safety
        }

        overlayed_image.to_rgb(); // If WB, make RGB

        imageMutex.unlock();
    }

    void initOverlay()
    {
        geofile = geodetic::projection::proj_file::readReferenceFile(std::string(projection::overlay_georef));

        loadImage();

        // Init texture
        textureID = makeImageTexture();
        textureBuffer = new uint32_t[overlayed_image.width() * overlayed_image.height()];
        uchar_to_rgba(overlayed_image.data(), textureBuffer, overlayed_image.width() * overlayed_image.height(), 3);
        updateImageTexture(textureID, textureBuffer, overlayed_image.width(), overlayed_image.height());

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

    void destroyOverlay()
    {
        deleteImageTexture(textureID);
        delete[] textureBuffer;
        overlayed_image = image::Image<uint8_t>(1, 1, 3);
        satdumpUiStatus = MAIN_MENU;
    }

    void doRender()
    {
        loadImage(); // Reset

        if (invert_image)
            overlayed_image.mirror(true, true);

        std::function<std::pair<int, int>(float, float, int, int)> projectionFunc;

        std::shared_ptr<geodetic::projection::LEOScanProjector> leoprojector;
        std::shared_ptr<geodetic::projection::GEOProjector> geoprojector;
        if (geofile->file_type == geodetic::projection::proj_file::LEO_TYPE)
        {
            geodetic::projection::proj_file::LEO_GeodeticReferenceFile leofile = *((geodetic::projection::proj_file::LEO_GeodeticReferenceFile *)geofile.get());
            std::shared_ptr<geodetic::projection::LEOScanProjectorSettings> settings = leoProjectionRefFile(leofile);
            leoprojector = std::make_shared<geodetic::projection::LEOScanProjector>(settings);
            leoprojector->setup_forward(98, 2, 16, 10);
            projectionFunc = [&leoprojector](float lat, float lon, int map_height, int map_width) -> std::pair<int, int>
            {
                int x;
                int y;
                leoprojector->forward({lat, lon, 0}, x, y);

                if (x < 0 || x > map_width)
                    return {-1, -1};
                if (y < 0 || y > map_height)
                    return {-1, -1};

                if (invert_image)
                {
                    x = (map_width - 1) - x;
                    y = (map_height - 1) - y;
                }

                return {x, y};
            };
        }
        else if (geofile->file_type == geodetic::projection::proj_file::GEO_TYPE)
        {
            geodetic::projection::proj_file::GEO_GeodeticReferenceFile gsofile = *((geodetic::projection::proj_file::GEO_GeodeticReferenceFile *)geofile.get());
            geoprojector = std::make_shared<geodetic::projection::GEOProjector>(geodetic::projection::proj_file::geoProjectionRefFile(gsofile));

            projectionFunc = [&geoprojector](float lat, float lon, int map_height, int map_width) -> std::pair<int, int>
            {
                int x;
                int y;
                geoprojector->forward(lon, lat, x, y);

                if (x < 0 || x > map_width)
                    return {-1, -1};
                if (y < 0 || y > map_height)
                    return {-1, -1};

                if (invert_image)
                {
                    x = (map_width - 1) - x;
                    y = (map_height - 1) - y;
                }

                return {x, y};
            };
        }
        else
        {
            return;
        }

        // Should we draw borders?
        if (draw_borders)
        {
            unsigned char color[3] = {0, 255, 0};
            map::drawProjectedMapShapefile({resources::getResourcePath("maps/ne_10m_admin_0_countries.shp")}, overlayed_image, color, projectionFunc, 200);
        }

        // Should we draw cities?
        if (draw_cities)
        {
            unsigned char color[3] = {255, 0, 0};
            map::drawProjectedCapitalsGeoJson({resources::getResourcePath("maps/ne_10m_populated_places_simple.json")}, overlayed_image, color, projectionFunc, cities_size_ratio);
        }

        // Should we draw custom labels?
        if (draw_custom_labels)
        {
            unsigned char color[3] = {0, 0, 255};
            map::drawProjectedLabels(customLabels, overlayed_image, color, projectionFunc, cities_size_ratio);
        }

        // Finally, update image
        //uchar_to_rgba(projected_image.data(), textureBuffer, output_width * output_height, 3);
        //updateImageTexture(textureID, textureBuffer, output_width, output_height);
    };

    void renderOverlay(int wwidth, int wheight)
    {
        if (rendering || isRenderDone)
        {
            // When rendering, update more often
            imageMutex.lock();
            uchar_to_rgba(overlayed_image.data(), textureBuffer, overlayed_image.width() * overlayed_image.height(), 3);
            updateImageTexture(textureID, textureBuffer, overlayed_image.width(), overlayed_image.height());
            imageMutex.unlock();

            if (isRenderDone > 0)
            {
                isRenderDone--;
            }
        }

        ImGui::SetNextWindowPos({0, 0});
        ImGui::SetNextWindowSize({(float)wwidth, (float)wheight});
        ImGui::Begin("Overlay Image", NULL, NOWINDOW_FLAGS | ImGuiWindowFlags_NoTitleBar);
        {
            float ratioX = ImGui::GetWindowWidth() / overlayed_image.width();
            float ratioY = (ImGui::GetWindowHeight() - 16) / overlayed_image.height();
            float ratio = ratioX > ratioY ? ratioY : ratioX;
            ImGui::SetCursorPos({(ImGui::GetWindowWidth() / 2) - (overlayed_image.width() * ratio) / 2, (ImGui::GetWindowHeight() / 2) - (overlayed_image.height() * ratio) / 2});
            ImGui::Image((void *)(intptr_t)textureID, {overlayed_image.width() * ratio, overlayed_image.height() * ratio});
        }
        ImGui::End();

        if (isFirstUiRun)
            ImGui::SetNextWindowSize({400 * ui_scale, 270 * ui_scale});
        ImGui::Begin("Overlay");
        {

            ImGui::Checkbox("Invert Image", &invert_image);
            ImGui::Spacing();
            ImGui::Checkbox("Draw Borders", &draw_borders);
            ImGui::Checkbox("Draw Cities", &draw_cities);
            ImGui::Checkbox("Draw Labels", &draw_custom_labels);
            ImGui::InputFloat("Cities Scale", &cities_size_ratio);
            if (ImGui::Button("Edit Custom Labels"))
                show_custom_labels_window = true;

            if (!rendering)
            {
                if (ImGui::Button("Overlay"))
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
                ImGui::Button("Processing...");

            ImGui::SameLine();

            if (ImGui::Button("Save"))
            {
#ifdef __APPLE__
                overlayed_image.save_png("overlayed.png");
#else
                logger->debug("Opening file dialog");
                std::string output_file = selectOutputFileDialog("Save projection to", "overlayed.png", {"*.png"});

                logger->info("Saving to " + output_file);

                if (output_file.size() > 0)
                {
                    overlayed_image.save_png(output_file.c_str());
                }
#endif
            }

            ImGui::SameLine();

            if (ImGui::Button("Exit"))
            {
                destroyOverlay();
            }

            ImGui::Text("On the first time, processing will take longer as some \n"
                        "equations have to be solved for applying the overlay. \n");
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

        if (isFirstUiRun)
            isFirstUiRun = false;
    }
};
