#include "main_ui.h"
#include "imgui/imgui_flags.h"
#include "imgui/imgui.h"
#include "processing.h"
#include "offline.h"
#include "settings.h"
#include "error.h"
#include "satdump_vars.h"
#include "core/config.h"
#include "style.h"

#include "app.h"

namespace satdump
{
    std::shared_ptr<Application> current_app;

    bool in_app = false;

    void initMainUI()
    {
        offline::setup();
        settings::setup();

        light_theme = config::main_cfg["user_interface"]["light_theme"]["value"].get<bool>();
        float manual_dpi_scaling = config::main_cfg["user_interface"]["manual_dpi_scaling"]["value"].get<float>();

        ui_scale = manual_dpi_scaling;

        // Setup Theme
        if (std::filesystem::exists("Roboto-Medium.ttf"))
        {
            if (light_theme)
                style::setLightStyle(".", manual_dpi_scaling);
            else
                style::setDarkStyle(".", manual_dpi_scaling);
        }
        else
        {
            if (light_theme)
                style::setLightStyle((std::string)RESOURCES_PATH);
            else
                style::setDarkStyle((std::string)RESOURCES_PATH);
        }

        registerApplications();
        current_app = application_registry["viewer"]();
    }

    void renderMainUI(int wwidth, int wheight)
    {
        if (in_app)
        {
            ImGui::SetNextWindowPos({0, 0});
            ImGui::SetNextWindowSize({(float)wwidth, (float)wheight});
            ImGui::Begin("Main", __null, NOWINDOW_FLAGS | ImGuiWindowFlags_NoDecoration);
            current_app->draw();
            ImGui::End();
        }
        else if (processing::is_processing)
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
                if (ImGui::BeginTabItem("Applications"))
                {
                    //current_app->draw();

                    for (std::pair<const std::string, std::function<std::shared_ptr<Application>()>> &appEntry : application_registry)
                    {
                        if (ImGui::Button(appEntry.first.c_str()))
                        {
                            in_app = true;
                            current_app = application_registry[appEntry.first]();
                        }
                    }

                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Settings"))
                {
                    settings::render();
                    ImGui::EndTabItem();
                }
            }
            ImGui::EndTabBar();
            ImGui::End();

            if (error::hasError)
                error::render();
        }
    }

    bool light_theme;

    ctpl::thread_pool ui_thread_pool(8);
}