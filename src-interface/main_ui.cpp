#include "main_ui.h"
#include "imgui/imgui_flags.h"
#include "imgui/imgui.h"
#include "processing.h"
#include "offline.h"
#include "settings.h"
#include "error.h"
#include "satdump_vars.h"
#include "core/config.h"
#include "core/style.h"
#include "resources.h"
#include "common/widgets/markdown_helper.h"

namespace satdump
{
    std::shared_ptr<RecorderApplication> recorder_app;
    std::shared_ptr<ViewerApplication> viewer_app;

    bool in_app = false; // true;

    widgets::MarkdownHelper credits_md;

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
                style::setLightStyle(satdump::RESPATH);
            else
                style::setDarkStyle(satdump::RESPATH);
        }

        // Load credits MD
        std::ifstream ifs(resources::getResourcePath("credits.md"));
        std::string credits_markdown((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
        credits_md.set_md(credits_markdown);
        credits_md.init();

        registerApplications();
        registerViewerHandlers();

        recorder_app = std::make_shared<RecorderApplication>();
        viewer_app = std::make_shared<ViewerApplication>();
    }

    void exitMainUI()
    {
        recorder_app->save_settings();
    }

    void renderMainUI(int wwidth, int wheight)
    {
        /*if (in_app)
        {
            ImGui::SetNextWindowPos({0, 0});
            ImGui::SetNextWindowSize({(float)wwidth, (float)wheight});
            ImGui::Begin("Main", NULL, NOWINDOW_FLAGS | ImGuiWindowFlags_NoDecoration);
            current_app->draw();
            ImGui::End();
        }*/
        /*else if (processing::is_processing)
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
        }*/
        // else
        {
            ImGui::SetNextWindowPos({0, 0});
            ImGui::SetNextWindowSize({(float)wwidth, (float)wheight});
            ImGui::Begin("Main", NULL, NOWINDOW_FLAGS | ImGuiWindowFlags_NoDecoration);
            if (ImGui::BeginTabBar("Main TabBar", ImGuiTabBarFlags_None))
            {
                if (ImGui::BeginTabItem("Offline processing"))
                {
                    if (processing::is_processing)
                    {
                        // ImGui::BeginChild("OfflineProcessingChild");
                        processing::ui_call_list_mutex->lock();
                        int live_width = wwidth; // ImGui::GetWindowWidth();
                        int live_height = ImGui::GetWindowHeight() - ImGui::GetCursorPos().y;
                        float winheight = processing::ui_call_list->size() > 0 ? live_height / processing::ui_call_list->size() : live_height;
                        float currentPos = ImGui::GetCursorPos().y;
                        for (std::shared_ptr<ProcessingModule> module : *processing::ui_call_list)
                        {
                            ImGui::SetNextWindowPos({0, currentPos});
                            currentPos += winheight;
                            ImGui::SetNextWindowSize({(float)live_width, (float)winheight});
                            module->drawUI(true);
                        }
                        processing::ui_call_list_mutex->unlock();
                        // ImGui::EndChild();
                    }
                    else
                    {
                        offline::render();
                    }

                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Recorder"))
                {
                    recorder_app->draw();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Viewer"))
                {
                    viewer_app->draw();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Applications"))
                {
                    // current_app->draw();
                    if (in_app)
                    {
                        // current_app->draw();
                    }
                    else
                    {

                        for (std::pair<const std::string, std::function<std::shared_ptr<Application>()>> &appEntry : application_registry)
                        {
                            if (ImGui::Button(appEntry.first.c_str()))
                            {
                                in_app = true;
                                // current_app = application_registry[appEntry.first]();
                            }
                        }
                    }

                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Settings"))
                {
                    settings::render();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Credits"))
                {
                    credits_md.render();
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