#pragma once

#include "common/tile_map/map.h"
#include "dll_export.h"
#include "explorer/explorer.h"
#include "libs/ctpl/ctpl_stl.h"
#include "recorder/recorder.h"
#include <string>

namespace satdump
{
    SATDUMP_DLL2 extern bool update_ui;
    SATDUMP_DLL2 extern ctpl::thread_pool ui_thread_pool;

    SATDUMP_DLL2 extern std::shared_ptr<explorer::ExplorerApplication> explorer_app;

    void initMainUI();
    void exitMainUI();
    void renderMainUI();

    // TODOREWORK move into another namespace?
    struct SetIsProcessingEvent
    {
    };

    struct SetIsDoneProcessingEvent
    {
    };
} // namespace satdump
