#pragma once

#include <string>
#include "libs/ctpl/ctpl_stl.h"
#include "app.h"
#include "viewer/viewer.h"
#include "recorder/recorder.h"
#include "common/tile_map/map.h"

namespace satdump
{
    extern ctpl::thread_pool ui_thread_pool;

    extern std::shared_ptr<RecorderApplication> recorder_app;
    extern std::shared_ptr<ViewerApplication> viewer_app;

    void initMainUI(float device_scale);
    void updateUI(float device_scale);
    void exitMainUI();
    void renderMainUI();
}
