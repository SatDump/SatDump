#pragma once

enum satdump_ui_status
{
    OFFLINE_PROCESSING,
    LIVE_PROCESSING,
    BASEBAND_RECORDER,
    MAIN_MENU
};

extern satdump_ui_status satdumpUiStatus;

void initMainUI();
void renderMainUI(int wwidth, int wheight);