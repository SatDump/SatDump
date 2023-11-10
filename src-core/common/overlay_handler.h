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

public:
    OverlayHandler()
    {
        set_defaults();
    }

    int enabled();
    bool drawUI();
    void apply(image::Image<uint16_t> &img, std::function<std::pair<int, int>(float, float, int, int)> &proj_func, float *step_cnt = nullptr);
    nlohmann::json get_config();
    void set_config(nlohmann::json in, bool status = true);
};