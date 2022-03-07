#include "main_ui.h"
//#include "settingsui.h"
//#include "recorder/recorder_menu.h"
//#include "recorder/recorder.h"
//#include "credits/credits.h"
#include "global.h"
#include "imgui/imgui_flags.h"
#include "imgui/imgui.h"
//#include "offline/offline.h"
#include "processing.h"
//#include "live/live.h"
//#include "live/live_run.h"
//#include "projection/projection.h"
//#include "projection/projection_menu.h"
//#include "projection/overlay.h"

// satdump_ui_status satdumpUiStatus = MAIN_MENU;

#include "offline.h"

void initMainUI()
{
    offline::setup();
}

bool isProcessing = false;

void renderMainUI(int wwidth, int wheight)
{
    if (isProcessing)
    {
        uiCallListMutex->lock();
        float winheight = uiCallList->size() > 0 ? wheight / uiCallList->size() : wheight;
        float currentPos = 0;
        for (std::shared_ptr<ProcessingModule> module : *uiCallList)
        {
            ImGui::SetNextWindowPos({0, currentPos});
            currentPos += winheight;
            ImGui::SetNextWindowSize({(float)wwidth, (float)winheight});
            module->drawUI(false);
        }
        uiCallListMutex->unlock();
    }
    else
    {
        ImGui::SetNextWindowPos({0, 0});
        ImGui::SetNextWindowSize({wwidth, wheight});
        ImGui::Begin("Main", __null, NOWINDOW_FLAGS | ImGuiWindowFlags_NoDecoration);

        if (ImGui::BeginTabBar("Main TabBar", ImGuiTabBarFlags_None))
        {
            if (ImGui::BeginTabItem("Offline processing"))
            {
                offline::render();
                ImGui::EndTabItem();
            }
        }
        ImGui::EndTabBar();

        ImGui::End();
    }
}