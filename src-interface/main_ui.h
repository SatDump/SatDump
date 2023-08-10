#pragma once

#include <string>
#include "libs/ctpl/ctpl_stl.h"
#include "app.h"
#include "viewer/viewer.h"
#include "recorder/recorder.h"
#include "common/tile_map/map.h"

namespace satdump
{
    extern std::string error_message;
    extern bool isError_UI;

    extern ctpl::thread_pool ui_thread_pool;

    extern bool light_theme;
    extern bool status_bar;

    extern std::shared_ptr<RecorderApplication> recorder_app;
    extern std::shared_ptr<ViewerApplication> viewer_app;

    void initMainUI();
    void exitMainUI();
    void renderMainUI(int wwidth, int wheight);
}