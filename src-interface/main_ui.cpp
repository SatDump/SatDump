#include "main_ui.h"
#include "settingsui.h"
#include "recorder/recorder_menu.h"
#include "recorder/recorder.h"
#include "credits/credits.h"
#include "global.h"
#include "imgui/imgui_flags.h"
#include "imgui/imgui.h"
#include "offline/offline.h"
#include "processing.h"
#include "live/live.h"
#include "live/live_run.h"
#include "projection/projection.h"
#include "projection/projection_menu.h"
#include "projection/overlay.h"

satdump_ui_status satdumpUiStatus = MAIN_MENU;

#ifdef BUILD_TRACKING
#include "tracking/tracking.h"

std::shared_ptr<TrackingUI> ui;
bool renderTracking = false;
#endif

void initMainUI()
{
    offline::initOfflineProcessingMenu();
    projection::initProjectionMenu();

#ifdef BUILD_LIVE
    live::initLiveProcessingMenu();
    findRadioDevices();
#endif
}

bool first = true;

void renderMainUI(int wwidth, int wheight)
{
#ifdef BUILD_TRACKING
    if (renderTracking)
    {
        if (first)
        {
            ui = std::make_shared<TrackingUI>();
            first = false;
        }
        ui->draw(wwidth, wheight);
        return;
    }
#endif

    if (satdumpUiStatus == OFFLINE_PROCESSING)
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
#ifdef BUILD_LIVE
    else if (satdumpUiStatus == LIVE_PROCESSING)
    {
        live::renderLive();
    }
    else if (satdumpUiStatus == BASEBAND_RECORDER)
    {
        recorder::renderRecorder(wwidth, wheight);
    }
#endif
    else if (satdumpUiStatus == PROJECTION)
    {
        projection::renderProjection(wwidth, wheight);
    }
    else if (satdumpUiStatus == OVERLAY)
    {
        projection_overlay::renderOverlay(wwidth, wheight);
    }
    else if (satdumpUiStatus == MAIN_MENU)
    {
        ImGui::SetNextWindowPos({0, 0});
        ImGui::SetNextWindowSize({(float)wwidth, (float)wheight});
        ImGui::Begin("Main Window", NULL, NOWINDOW_FLAGS | ImGuiWindowFlags_NoTitleBar);

        if (ImGui::BeginTabBar("Main TabBar", ImGuiTabBarFlags_None))
        {
            if (ImGui::BeginTabItem("Offline processing"))
            {
                offline::renderOfflineProcessingMenu();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Live processing"))
            {
#ifdef BUILD_LIVE
                live::renderLiveProcessingMenu();
#else
                ImGui::Text("Live support was not enabled in this build. Please rebuild with BUILD_LIVE=ON if you wish to use it.");
#endif
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Baseband Recorder"))
            {
#ifdef BUILD_LIVE
                recorder::renderRecorderMenu();
#else
                ImGui::Text("Live support was not enabled in this build. Please rebuild with BUILD_LIVE=ON if you wish to use it.");
#endif
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Projection"))
            {
                projection::renderProjectionMenu(wwidth, wheight);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Settings"))
            {
                renderSettings(wwidth, wheight);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Credits"))
            {
                credits::renderCredits(wwidth, wheight);
                ImGui::EndTabItem();
            }
#ifdef BUILD_TRACKING
            if (ImGui::BeginTabItem("Tracking"))
            {
                renderTracking = true;
                ImGui::EndTabItem();
            }
#endif
        }
        ImGui::EndTabBar();

        ImGui::End();
    }
}