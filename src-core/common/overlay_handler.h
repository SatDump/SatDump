#pragma once

#include "nlohmann/json.hpp"
#include "imgui/imgui.h"
#include "common/map/map_drawer.h"
#include "nlohmann/json_utils.h"
#include "logger.h"
#include "core/resources.h"
#include "common/widgets/stepped_slider.h"

struct OverlayCache
{
    size_t width = 0;
    size_t height = 0;
    std::vector<size_t> map;
};

struct GrayscaleOverlayCache
{
    size_t width = 0;
    size_t height = 0;
    std::map<size_t, float> map;
};

class OverlayHandler
{
private:
    OverlayCache map_cache;
    OverlayCache shores_cache;
    OverlayCache latlon_cache;
    GrayscaleOverlayCache cities_cache;
    GrayscaleOverlayCache qth_cache;

    image::TextDrawer text_drawer;

public:
    // Colors
    ImVec4 color_borders = {0, 1, 0, 1};
    ImVec4 color_shores = {1, 1, 0, 1};
    ImVec4 color_cities = {1, 0, 0, 1};
    ImVec4 color_qth = {1, 0, 1, 1};
    ImVec4 color_latlon = {0, 0, 1, 1};

    // QTH Label
    std::string qth_label;
    std::string last_qth_label = "";

    // Settings
    bool draw_map_overlay = false;
    bool draw_shores_overlay = false;
    bool draw_cities_overlay = false;
    bool draw_qth_overlay = false;
    bool draw_latlon_overlay = false;

    int cities_type = 0;
    int cities_size = 50;
    int cities_scale_rank = 3;
    int last_cities_type = -1;
    int last_cities_size = -1;
    int last_cities_scale_rank = -1;

    void set_defaults();
    void clear_cache();

public:
    OverlayHandler()
    {
        set_defaults();
    }

    int enabled();
    bool drawUI();
    void apply(image::Image &img, std::function<std::pair<int, int>(double, double, int, int)> proj_func, float *step_cnt = nullptr);
    nlohmann::json get_config();
    void set_config(nlohmann::json in, bool status = true);
};