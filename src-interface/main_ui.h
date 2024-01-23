#pragma once

#include <string>
#include "libs/ctpl/ctpl_stl.h"
#include "app.h"
#include "viewer/viewer.h"
#include "recorder/recorder.h"
#include "common/tile_map/map.h"

namespace satdump
{
    extern bool update_ui;
    extern ctpl::thread_pool ui_thread_pool;

    extern std::shared_ptr<RecorderApplication> recorder_app;
    extern std::shared_ptr<ViewerApplication> viewer_app;

    void initMainUI();
    void exitMainUI();
    void renderMainUI();
}
