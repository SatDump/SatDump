#include "overlay_handler.h"
#include "core/config.h"
#include "imgui/imgui_stdlib.h"

int OverlayHandler::enabled()
{
    return (int)draw_map_overlay +
           (int)draw_cities_overlay +
           (int)draw_qth_overlay +
           (int)draw_latlon_overlay +
           (int)draw_shores_overlay;
}

bool OverlayHandler::drawUI()
{
    bool update = false;
    ImGui::Checkbox("Lat/Lon Grid", &draw_latlon_overlay);
    ImGui::SameLine();
    ImGui::ColorEdit3("##latlongrid", (float *)&color_latlon, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);

    ImGui::Checkbox("Map Overlay##Projs", &draw_map_overlay);
    ImGui::SameLine();
    ImGui::ColorEdit3("##borders", (float *)&color_borders, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);

    ImGui::Checkbox("Shores Overlay##Projs", &draw_shores_overlay);
    ImGui::SameLine();
    ImGui::ColorEdit3("##shores", (float *)&color_shores, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);

    ImGui::Checkbox("Cities Overlay##Projs", &draw_cities_overlay);
    ImGui::SameLine();
    ImGui::ColorEdit3("##cities##Projs", (float *)&color_cities, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);

    ImGui::Checkbox("QTH Overlay##Projs", &draw_qth_overlay);
    ImGui::SameLine();
    ImGui::ColorEdit3("##qth##Projs", (float *)&color_qth, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
    if (draw_qth_overlay)
        ImGui::InputText("QTH Label##Projs", &qth_label);

    widgets::SteppedSliderInt("Map Labels Font Size", &cities_size, 10, 500);
    static const char *city_categories[] = {"Capitals Only", "Capitals + Regional Capitals", "All (by Scale Rank)"};
    ImGui::Combo("Cities Type##Projs", &cities_type, city_categories, IM_ARRAYSIZE(city_categories));
    if (cities_type == 2)
        widgets::SteppedSliderInt("Cities Scale Rank", &cities_scale_rank, 0, 10);

    if (ImGui::Button("Set Defaults###oerlayhandlers"))
        set_defaults();
    ImGui::SameLine();
    if (ImGui::Button("Apply###overlayhandlerapply"))
        update = true;

    return update;
}

void OverlayHandler::apply(image2::Image &img, std::function<std::pair<int, int>(double, double, int, int)> proj_func, float *step_cnt)
{
    const size_t width = img.width();
    const size_t height = img.height();
    const int channels = img.channels();

    // Draw map borders
    if (draw_map_overlay)
    {
        if (map_cache.map.size() == 0 || map_cache.width != width || map_cache.height != height)
        {
            logger->info("Drawing map overlay...");
            map_cache.map.clear();
            map_cache.width = width;
            map_cache.height = height;
            image2::Image bitmask(8, width, height, 1);
            map::drawProjectedMapShapefile({resources::getResourcePath("maps/ne_10m_admin_0_countries.shp")},
                                           bitmask,
                                           {1},
                                           proj_func);
            for (size_t i = 0; i < bitmask.size(); i++)
                if (bitmask.get(i) == 255)
                    map_cache.map.push_back(i);
        }
        else
            logger->info("Applying cached map overlay...");

        double color[4] = {color_borders.x, color_borders.y, color_borders.z, 1};
        for (size_t &location : map_cache.map)
            for (int i = 0; i < channels; i++)
                img.setf(i, location, color[i]);

        if (step_cnt != nullptr)
            (*step_cnt)++;
    }

    // Draw map shorelines
    if (draw_shores_overlay)
    {
        if (shores_cache.map.size() == 0 || shores_cache.width != width || shores_cache.height != height)
        {
            logger->info("Drawing shores overlay...");
            shores_cache.map.clear();
            shores_cache.width = width;
            shores_cache.height = height;
            image2::Image bitmask(8, width, height, 1);

            map::drawProjectedMapShapefile({resources::getResourcePath("maps/ne_10m_coastline.shp")},
                                           bitmask,
                                           {1},
                                           proj_func);
            for (size_t i = 0; i < bitmask.size(); i++)
                if (bitmask.get(i) == 255)
                    shores_cache.map.push_back(i);
        }
        else
            logger->info("Applying cached shores overlay...");

        double color[4] = {color_shores.x, color_shores.y, color_shores.z, 1};
        for (size_t &location : shores_cache.map)
            for (int i = 0; i < channels; i++)
                img.setf(i, location, color[i]);

        if (step_cnt != nullptr)
            (*step_cnt)++;
    }

    // Draw cities points
    if (draw_cities_overlay)
    {
        if (cities_cache.map.size() == 0 || cities_cache.width != width || cities_cache.height != height ||
            last_cities_size != cities_size || last_cities_type != cities_type || last_cities_scale_rank != cities_scale_rank)
        {
            logger->info("Drawing cities overlay...");
            cities_cache.map.clear();
            cities_cache.width = width;
            cities_cache.height = height;
            image2::Image bitmask(8, width, height, 1);
            text_drawer.init_font(resources::getResourcePath("fonts/font.ttf"));
            map::drawProjectedCitiesGeoJson({resources::getResourcePath("maps/ne_10m_populated_places_simple.json")},
                                            bitmask,
                                            text_drawer,
                                            {1},
                                            proj_func,
                                            cities_size,
                                            cities_type,
                                            cities_scale_rank);
            for (size_t i = 0; i < bitmask.size(); i++)
                if (bitmask.get(i) > 0)
                    cities_cache.map.insert({i, (float)bitmask.getf(i)});
        }
        else
            logger->info("Applying cached cities overlay...");

        float color[3] = {color_cities.x, color_cities.y, color_cities.z};
        for (auto &location : cities_cache.map)
        {
            float src_alpha = channels == 4 ? (float)img.getf(3, location.first) : 1.0f;
            float alpha = location.second + src_alpha * (1.0f - location.second);
            for (int i = 0; i < 3; i++)
                img.setf(i, location.first, (((color[i] * location.second) + (((float)img.getf(i, location.first)) * src_alpha * (1.0f - location.second))) / alpha));

            if (channels == 4)
                img.setf(3, location.first, alpha);
            else
                for (int i = 0; i < channels; i++)
                    img.setf(i, location.first, (float)img.getf(i, location.first) * alpha);
        }

        if (step_cnt != nullptr)
            (*step_cnt)++;
    }

    // Draw qth overlay
    if (draw_qth_overlay)
    {
        if (qth_cache.map.size() == 0 || qth_cache.width != width || qth_cache.height != height ||
            last_cities_size != cities_size || last_qth_label != qth_label)
        {
            logger->info("Drawing QTH overlay...");
            qth_cache.map.clear();
            qth_cache.width = width;
            qth_cache.height = height;
            last_qth_label = qth_label;
            image2::Image bitmask(8, width, height, 1);
            text_drawer.init_font(resources::getResourcePath("fonts/font.ttf"));
            double qth_lon = satdump::config::main_cfg["satdump_general"]["qth_lon"]["value"].get<double>();
            double qth_lat = satdump::config::main_cfg["satdump_general"]["qth_lat"]["value"].get<double>();
            std::pair<float, float> cc = proj_func(qth_lat, qth_lon, height, width);

            if (!(cc.first == -1 || cc.second == -1))
            {
                bitmask.draw_line(cc.first - cities_size * 0.3, cc.second - cities_size * 0.3, cc.first + cities_size * 0.3, cc.second + cities_size * 0.3, {1});
                bitmask.draw_line(cc.first + cities_size * 0.3, cc.second - cities_size * 0.3, cc.first - cities_size * 0.3, cc.second + cities_size * 0.3, {1});
                bitmask.draw_circle(cc.first, cc.second, 0.15 * cities_size, {1}, true);
                text_drawer.draw_text(bitmask, cc.first, cc.second + cities_size * 0.15, {1}, cities_size, qth_label);
            }

            for (size_t i = 0; i < bitmask.size(); i++)
                if (bitmask.get(i) > 0)
                    qth_cache.map.insert({i, (float)bitmask.getf(i)});
        }
        else
            logger->info("Applying cached QTH overlay...");

        float color[3] = {color_qth.x, color_qth.y, color_qth.z};
        for (auto &location : qth_cache.map)
        {
            float src_alpha = channels == 4 ? (float)img.getf(3, location.first) : 1.0f;
            float alpha = location.second + src_alpha * (1.0f - location.second);
            for (int i = 0; i < 3; i++)
                img.setf(i, location.first, (((color[i] * location.second) + (((float)img.getf(i, location.first)) * src_alpha * (1.0f - location.second))) / alpha));

            if (channels == 4)
                img.setf(3, location.first, alpha);
            else
                for (int i = 0; i < channels; i++)
                    img.setf(i, location.first, (float)img.getf(i, location.first) * alpha);
        }

        if (step_cnt != nullptr)
            (*step_cnt)++;
    }

    // Draw latlon grid
    if (draw_latlon_overlay)
    {
        if (latlon_cache.map.size() == 0 || latlon_cache.width != width || latlon_cache.height != height)
        {
            logger->info("Drawing lat/lon overlay...");
            latlon_cache.map.clear();
            latlon_cache.width = width;
            latlon_cache.height = height;
            image2::Image bitmask(8, width, height, 1);
            map::drawProjectedMapLatLonGrid(bitmask, {1}, proj_func);

            for (size_t i = 0; i < bitmask.size(); i++)
                if (bitmask.get(i) == 255)
                    latlon_cache.map.push_back(i);
        }
        else
            logger->info("Applying cached lat/lon overlay...");

        double color[4] = {color_latlon.x, color_latlon.y, color_latlon.z, 1};
        for (size_t &location : latlon_cache.map)
            for (int i = 0; i < channels; i++)
                img.setf(i, location, color[i]);

        if (step_cnt != nullptr)
            (*step_cnt)++;
    }

    // Clean up
    last_cities_size = cities_size;
    last_cities_type = cities_type;
    last_cities_scale_rank = cities_scale_rank;
}

nlohmann::json OverlayHandler::get_config()
{
    nlohmann::json out;
    out["qth_label"] = qth_label;
    out["cities_type"] = cities_type;
    out["cities_size"] = cities_size;
    out["cities_scale_rank"] = cities_scale_rank;
    out["borders_color"] = {color_borders.x, color_borders.y, color_borders.z};
    out["shores_color"] = {color_shores.x, color_shores.y, color_shores.z};
    out["cities_color"] = {color_cities.x, color_cities.y, color_cities.z};
    out["qth_color"] = {color_qth.x, color_qth.y, color_qth.z};
    out["latlon_color"] = {color_latlon.x, color_latlon.y, color_latlon.z};

    out["draw_map_overlay"] = draw_map_overlay;
    out["draw_shores_overlay"] = draw_shores_overlay;
    out["draw_cities_overlay"] = draw_cities_overlay;
    out["draw_qth_overlay"] = draw_qth_overlay;
    out["cities_scale"] = cities_size;
    out["draw_latlon_overlay"] = draw_latlon_overlay;

    return out;
}

void OverlayHandler::set_config(nlohmann::json in, bool status)
{
    setValueIfExists(in["qth_label"], qth_label);
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

    if (in.contains("qth_color"))
    {
        std::vector<float> color = in["qth_color"].get<std::vector<float>>();
        color_qth.x = color[0];
        color_qth.y = color[1];
        color_qth.z = color[2];
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

void OverlayHandler::set_defaults()
{
    qth_label = satdump::config::main_cfg["satdump_general"]["default_qth_label"]["value"].get<std::string>();
    std::vector<float> dcolor_borders = satdump::config::main_cfg["satdump_general"]["default_borders_color"]["value"].get<std::vector<float>>();
    color_borders.x = dcolor_borders[0];
    color_borders.y = dcolor_borders[1];
    color_borders.z = dcolor_borders[2];
    std::vector<float> dcolor_shores = satdump::config::main_cfg["satdump_general"]["default_shores_color"]["value"].get<std::vector<float>>();
    color_shores.x = dcolor_shores[0];
    color_shores.y = dcolor_shores[1];
    color_shores.z = dcolor_shores[2];
    std::vector<float> dcolor_cities = satdump::config::main_cfg["satdump_general"]["default_cities_color"]["value"].get<std::vector<float>>();
    color_cities.x = dcolor_cities[0];
    color_cities.y = dcolor_cities[1];
    color_cities.z = dcolor_cities[2];
    std::vector<float> dcolor_qth = satdump::config::main_cfg["satdump_general"]["default_qth_color"]["value"].get<std::vector<float>>();
    color_qth.x = dcolor_qth[0];
    color_qth.y = dcolor_qth[1];
    color_qth.z = dcolor_qth[2];
    std::vector<float> dcolor_latlon = satdump::config::main_cfg["satdump_general"]["default_latlon_color"]["value"].get<std::vector<float>>();
    color_latlon.x = dcolor_latlon[0];
    color_latlon.y = dcolor_latlon[1];
    color_latlon.z = dcolor_latlon[2];
}

void OverlayHandler::clear_cache()
{
    map_cache.map.clear();
    shores_cache.map.clear();
    cities_cache.map.clear();
    qth_cache.map.clear();
    latlon_cache.map.clear();
}