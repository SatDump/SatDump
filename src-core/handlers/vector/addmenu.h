#pragma once

#include "common/map/shapefile.h"
#include "core/resources.h"
#include "handlers/handler.h"
#include "handlers/vector/shapefile_handler.h"
#include "logger.h"

namespace satdump
{
    namespace handlers
    {
        // TODORERWORK this is extremely temporary!!!!!!!!!
        inline void renderVectorOverlayMenu(handlers::Handler *h)
        {
            // TODOREWORK move out?!
            if (ImGui::BeginMenu("Add Overlay"))
            {
                if (ImGui::MenuItem("Shores"))
                    h->addSubHandler(std::make_shared<handlers::ShapefileHandler>(resources::getResourcePath("maps/ne_10m_coastline.shp")), true);

                if (ImGui::MenuItem("Borders"))
                    h->addSubHandler(std::make_shared<ShapefileHandler>(resources::getResourcePath("maps/ne_10m_admin_0_countries.shp")), true);

                if (ImGui::MenuItem("Cities"))
                    h->addSubHandler(std::make_shared<ShapefileHandler>(resources::getResourcePath("maps/ne_10m_populated_places_simple.shp")), true);

                if (ImGui::MenuItem("Lat/Lon Grid"))
                {
                    auto h2 = std::make_shared<ShapefileHandler>();
                    h2->shapefile_name = "Lat/Lon Grid";

                    shapefile::PolyLineRecord r;

                    for (float lon = -180; lon < 180; lon += 10)
                    {
                        r.parts_points.push_back(std::vector<shapefile::point_t>());
                        auto &l = r.parts_points[r.parts_points.size() - 1];

                        for (float lat = -90; lat < 90; lat += 0.05)
                            l.push_back({lon, lat});
                    }

                    for (float lat = -90; lat < 90; lat += 10)
                    {
                        r.parts_points.push_back(std::vector<shapefile::point_t>());
                        auto &l = r.parts_points[r.parts_points.size() - 1];

                        for (float lon = -180; lon < 180; lon += 0.05)
                            l.push_back({lon, lat});
                    }

                    h2->file = std::make_unique<shapefile::Shapefile>();
                    h2->file->polyline_records.push_back(r);

                    h->addSubHandler(h2, true);
                }

                ImGui::EndMenu();
            }
        }
    } // namespace handlers
} // namespace satdump