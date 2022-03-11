#include "main_ui.h"
#include "imgui/imgui_flags.h"
#include "imgui/imgui.h"
#include "processing.h"
#include "offline.h"
#include "error.h"

namespace satdump
{
    void initMainUI()
    {
        offline::setup();
    }

    std::string error_message;
    bool isError_UI = false;

    void renderMainUI(int wwidth, int wheight)
    {
        if (processing::is_processing)
        {
            processing::ui_call_list_mutex->lock();
            float winheight = processing::ui_call_list->size() > 0 ? wheight / processing::ui_call_list->size() : wheight;
            float currentPos = 0;
            for (std::shared_ptr<ProcessingModule> module : *processing::ui_call_list)
            {
                ImGui::SetNextWindowPos({0, currentPos});
                currentPos += winheight;
                ImGui::SetNextWindowSize({(float)wwidth, (float)winheight});
                module->drawUI(false);
            }
            processing::ui_call_list_mutex->unlock();
        }
        else
        {
            ImGui::SetNextWindowPos({0, 0});
            ImGui::SetNextWindowSize({(float)wwidth, (float)wheight});
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

            if (error::hasError)
                error::render();
        }
    }

    ctpl::thread_pool ui_thread_pool(8);
}