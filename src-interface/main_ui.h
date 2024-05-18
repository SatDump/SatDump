#pragma once

#include "dll_export.h"
#include <string>
#include "libs/ctpl/ctpl_stl.h"
#include "app.h"
#include "viewer/viewer.h"
#include "recorder/recorder.h"
#include "common/tile_map/map.h"

namespace satdump
{
    SATDUMP_DLL extern bool update_ui;
    SATDUMP_DLL extern ctpl::thread_pool ui_thread_pool;

    SATDUMP_DLL extern std::shared_ptr<RecorderApplication> recorder_app;
    SATDUMP_DLL extern std::shared_ptr<ViewerApplication> viewer_app;

    void initMainUI();
    void exitMainUI();
    void renderMainUI();
}
