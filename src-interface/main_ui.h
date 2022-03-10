#pragma once

#include <string>
#include "libs/ctpl/ctpl_stl.h"

namespace satdump
{
    extern std::string error_message;
    extern bool isError_UI;

    extern ctpl::thread_pool ui_thread_pool;

    void initMainUI();
    void renderMainUI(int wwidth, int wheight);
}