#include "overlay_handler.h"

int OverlayHandler::enabled()
{
    return (int)draw_map_overlay +
           (int)draw_cities_overlay +
           (int)draw_latlon_overlay +
           (int)draw_shores_overlay;
}

bool OverlayHandler::drawUI()
{
    bool update = false;
    update |= ImGui::Checkbox("Lat/Lon Grid", &draw_latlon_overlay);
    ImGui::SameLine();
    ImGui::ColorEdit3("##latlongrid", (float *)&color_latlon, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
    update |= ImGui::IsItemDeactivatedAfterEdit();

    update |= ImGui::Checkbox("Map Overlay##Projs", &draw_map_overlay);
    ImGui::SameLine();
    ImGui::ColorEdit3("##borders", (float *)&color_borders, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
    update |= ImGui::IsItemDeactivatedAfterEdit();

    update |= ImGui::Checkbox("Shores Overlay##Projs", &draw_shores_overlay);
    ImGui::SameLine();
    ImGui::ColorEdit3("##shores", (float *)&color_shores, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
    update |= ImGui::IsItemDeactivatedAfterEdit();

    update |= ImGui::Checkbox("Cities Overlay##Projs", &draw_cities_overlay);
    ImGui::SameLine();
    ImGui::ColorEdit3("##cities##Projs", (float *)&color_cities, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
    update |= ImGui::IsItemDeactivatedAfterEdit();
    update |= widgets::SteppedSliderInt("Cities Font Size", &cities_size, 10, 500);
    static const char *city_categories[] = {"Capitals Only", "Capitals + Regional Capitals", "All (by Scale Rank)"};
    update |= ImGui::Combo("Cities Type##Projs", &cities_type, city_categories, IM_ARRAYSIZE(city_categories));
    if (cities_type == 2)
        update |= widgets::SteppedSliderInt("Cities Scale Rank", &cities_scale_rank, 0, 10);
    return update;
}

void OverlayHandler::apply(image::Image<uint16_t> &img, std::function<std::pair<int, int>(float, float, int, int)> &proj_func, float *step_cnt)
{
    // Draw map borders
    if (draw_map_overlay)
    {
        logger->info("Drawing map overlay...");
        unsigned short color[4] = {(unsigned short)(color_borders.x * 65535.0f), (unsigned short)(color_borders.y * 65535.0f), (unsigned short)(color_borders.z * 65535.0f), 65535};
        map::drawProjectedMapShapefile({resources::getResourcePath("maps/ne_10m_admin_0_countries.shp")},
                                       img,
                                       color,
                                       proj_func);
        if (step_cnt != nullptr)
            (*step_cnt)++;
    }

    // Draw map shorelines
    if (draw_shores_overlay)
    {
        logger->info("Drawing shores overlay...");
        unsigned short color[4] = {(unsigned short)(color_shores.x * 65535.0f), (unsigned short)(color_shores.y * 65535.0f), (unsigned short)(color_shores.z * 65535.0f), 65535};
        map::drawProjectedMapShapefile({resources::getResourcePath("maps/ne_10m_coastline.shp")},
                                       img,
                                       color,
                                       proj_func);
        if (step_cnt != nullptr)
            (*step_cnt)++;
    }

    // Draw cities points
    if (draw_cities_overlay)
    {
        logger->info("Drawing map overlay...");
        unsigned short color[4] = {(unsigned short)(color_cities.x * 65535.0f), (unsigned short)(color_cities.y * 65535.0f), (unsigned short)(color_cities.z * 65535.0f), 65535};
        map::drawProjectedCitiesGeoJson({resources::getResourcePath("maps/ne_10m_populated_places_simple.json")},
                                        img,
                                        color,
                                        proj_func,
                                        cities_size,
                                        cities_type,
                                        cities_scale_rank);
        if (step_cnt != nullptr)
            (*step_cnt)++;
    }

    // Draw latlon grid
    if (draw_latlon_overlay)
    {
        logger->info("Drawing latlon overlay...");
        unsigned short color[4] = {(unsigned short)(color_latlon.x * 65535.0f), (unsigned short)(color_latlon.y * 65535.0f), (unsigned short)(color_latlon.z * 65535.0f), 65535};
        map::drawProjectedMapLatLonGrid(img,
                                        color,
                                        proj_func);
        if (step_cnt != nullptr)
            (*step_cnt)++;
    }
}

nlohmann::json OverlayHandler::get_config()
{
    nlohmann::json out;
    out["cities_type"] = cities_type;
    out["cities_size"] = cities_size;
    out["cities_scale_rank"] = cities_scale_rank;
    out["borders_color"] = {color_borders.x, color_borders.y, color_borders.z};
    out["shores_color"] = {color_shores.x, color_shores.y, color_shores.z};
    out["cities_color"] = {color_cities.x, color_cities.y, color_cities.z};
    out["latlon_color"] = {color_latlon.x, color_latlon.y, color_latlon.z};

    out["draw_map_overlay"] = draw_map_overlay;
    out["draw_shores_overlay"] = draw_shores_overlay;
    out["draw_cities_overlay"] = draw_cities_overlay;
    out["cities_scale"] = cities_size;
    out["draw_latlon_overlay"] = draw_latlon_overlay;

    return out;
}

void OverlayHandler::set_config(nlohmann::json in, bool status)
{
    if (in.contains("borders_color"))
    {
        std::vector<float> color = in["borders_color"].get<std::vector<float>>();
        color_borders.x = color[0];
        color_borders.y = color[1];
        color_borders.z = color[2];
    }

    if (in.contains("shores_color"))
    {
        std::vector<float> color = in["shores_color"].get<std::vector<float>>();
        color_shores.x = color[0];
        color_shores.y = color[1];
        color_shores.z = color[2];
    }

    if (in.contains("cities_color"))
    {
        std::vector<float> color = in["cities_color"].get<std::vector<float>>();
        color_cities.x = color[0];
        color_cities.y = color[1];
        color_cities.z = color[2];
    }

    if (in.contains("latlon_color"))
    {
        std::vector<float> color = in["latlon_color"].get<std::vector<float>>();
        color_latlon.x = color[0];
        color_latlon.y = color[1];
        color_latlon.z = color[2];
    }

    if (in.contains("cities_size"))
        cities_size = in["cities_size"].get<int>();
    if (in.contains("cities_type"))
        cities_type = in["cities_type"].get<int>();
    if (in.contains("cities_scale_rank"))
        cities_scale_rank = in["cities_scale_rank"].get<int>();

    if (status)
    {
        setValueIfExists(in["draw_map_overlay"], draw_map_overlay);
        setValueIfExists(in["draw_shores_overlay"], draw_shores_overlay);
        setValueIfExists(in["draw_cities_overlay"], draw_cities_overlay);
        setValueIfExists(in["draw_latlon_overlay"], draw_latlon_overlay);
    }
    setValueIfExists(in["cities_scale"], cities_size);
}
