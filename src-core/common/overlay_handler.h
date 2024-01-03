#pragma once

#include "nlohmann/json.hpp"
#include "imgui/imgui.h"
#include "common/map/map_drawer.h"
#include "nlohmann/json_utils.h"
#include "logger.h"
#include "resources.h"
#include "common/widgets/stepped_slider.h"

class OverlayHandler
{
public:
    // Colors
    ImVec4 color_borders = {0, 1, 0, 1};
    ImVec4 color_shores = {1, 1, 0, 1};
    ImVec4 color_cities = {1, 0, 0, 1};
    ImVec4 color_qth = {1, 0, 1, 1};
    ImVec4 color_latlon = {0, 0, 1, 1};

    ImVec4 last_color_borders = { 0, 0, 0, 0 };
    ImVec4 last_color_shores = { 0, 0, 0, 0 };
    ImVec4 last_color_cities = { 0, 0, 0, 0 };
    ImVec4 last_color_qth = { 0, 0, 0, 0 };
    ImVec4 last_color_latlon = { 0, 0, 0, 0 };

    // Overlay caches
    image::Image<uint8_t> *map_cache = nullptr;
    image::Image<uint8_t> *shores_cache = nullptr;
    image::Image<uint8_t> *cities_cache = nullptr;
    image::Image<uint8_t> *qth_cache = nullptr;
    image::Image<uint8_t> *latlon_cache = nullptr;

    // Settings
    bool draw_map_overlay = false;
    bool draw_shores_overlay = false;
    bool draw_cities_overlay = false;
    bool draw_qth_overlay = false;
    bool draw_latlon_overlay = false;

    int cities_type = 0;
    int cities_size = 50;
    int cities_scale_rank = 3;

    void set_defaults();
    void clear_cache();

public:
    OverlayHandler()
    {
        set_defaults();
    }
    ~OverlayHandler()
    {
        clear_cache();
    }

    int enabled();
    bool drawUI();
    void apply(image::Image<uint16_t> &img, std::function<std::pair<int, int>(float, float, int, int)> &proj_func, float *step_cnt = nullptr);
    nlohmann::json get_config();
    void set_config(nlohmann::json in, bool status = true);
};