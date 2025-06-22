#pragma once

#include "dll_export.h"
#include <string>
#include "libs/ctpl/ctpl_stl.h"
#include "app.h"
#include "explorer/explorer.h"
#include "recorder/recorder.h"
#include "common/tile_map/map.h"

namespace satdump
{
    SATDUMP_DLL2 extern bool update_ui;
    SATDUMP_DLL2 extern ctpl::thread_pool ui_thread_pool;

    SATDUMP_DLL2 extern std::shared_ptr<RecorderApplication> recorder_app;
    SATDUMP_DLL2 extern std::shared_ptr<explorer::ExplorerApplication> explorer_app;

    void initMainUI();
    void exitMainUI();
    void renderMainUI();
}
