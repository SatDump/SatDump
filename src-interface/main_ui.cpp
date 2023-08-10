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
#include "common/audio/audio_sink.h"
#include "imgui_notify/imgui_notify.h"
#include "notify_logger_sink.h"
#include "status_logger_sink.h"

// #define ENABLE_DEBUG_MAP
#ifdef ENABLE_DEBUG_MAP
#include "common/widgets/image_view.h"
float lat = 0, lon = 0, lat1 = 0, lon1 = 0;
int zoom = 0;
image::Image<uint8_t> img(800, 400, 3);
ImageViewWidget ivw;
#endif

namespace satdump
{
    std::shared_ptr<RecorderApplication> recorder_app;
    std::shared_ptr<ViewerApplication> viewer_app;

    bool in_app = false; // true;

    widgets::MarkdownHelper credits_md;

    std::shared_ptr<NotifyLoggerSink> notify_logger_sink;
    std::shared_ptr<StatusLoggerSink> status_logger_sink;

    void initMainUI()
    {
        audio::registerSinks();

        offline::setup();
        settings::setup();

        light_theme = config::main_cfg["user_interface"]["light_theme"]["value"].get<bool>();
        float manual_dpi_scaling = config::main_cfg["user_interface"]["manual_dpi_scaling"]["value"].get<float>();
        status_bar = config::main_cfg["user_interface"]["status_bar"]["value"].get<float>();

#ifdef __ANDROID__
        manual_dpi_scaling *= 3; // Otherwise it's just too small by default!
#endif

        ui_scale = manual_dpi_scaling;

        // Setup Theme

        if (light_theme)
            style::setLightStyle(manual_dpi_scaling);
        else
            style::setDarkStyle(manual_dpi_scaling);

        // Load credits MD
        std::ifstream ifs(resources::getResourcePath("credits.md"));
        std::string credits_markdown((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
        credits_md.set_md(credits_markdown);
        credits_md.init();

        registerApplications();
        registerViewerHandlers();

        recorder_app = std::make_shared<RecorderApplication>();
        viewer_app = std::make_shared<ViewerApplication>();

        // Logger notify sink
        notify_logger_sink = std::make_shared<NotifyLoggerSink>();
        logger->add_sink(notify_logger_sink);

        //Logger status bar sync
        if (status_bar)
        {
            status_logger_sink = std::make_shared<StatusLoggerSink>();
            logger->add_sink(status_logger_sink);
        }
    }

    void exitMainUI()
    {
        viewer_app.reset();
        recorder_app->save_settings();
        recorder_app.reset();
    }

    bool main_ui_is_processing_selected = false;

    void renderMainUI(int wwidth, int wheight)
    {
        // ImGui::ShowDemoWindow();

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
            if(status_bar)
                wheight -= status_logger_sink->draw();
            ImGui::SetNextWindowPos({0, 0});
            ImGui::SetNextWindowSize({(float)wwidth, (processing::is_processing & main_ui_is_processing_selected) ? -1.0f : (float)wheight});
            ImGui::Begin("Main", NULL, NOWINDOW_FLAGS | ImGuiWindowFlags_NoDecoration);
            if (ImGui::BeginTabBar("Main TabBar", ImGuiTabBarFlags_None))
            {
                main_ui_is_processing_selected = false;

                if (ImGui::BeginTabItem("Offline processing"))
                {
                    if (processing::is_processing)
                    {
                        // ImGui::BeginChild("OfflineProcessingChild");
                        processing::ui_call_list_mutex->lock();
                        int live_width = wwidth; // ImGui::GetWindowWidth();
                        int live_height = /*ImGui::GetWindowHeight()*/ wheight - ImGui::GetCursorPos().y;
                        float winheight = processing::ui_call_list->size() > 0 ? live_height / processing::ui_call_list->size() : live_height;
                        float currentPos = ImGui::GetCursorPos().y;
                        for (std::shared_ptr<ProcessingModule> module : *processing::ui_call_list)
                        {
                            ImGui::SetNextWindowPos({0, currentPos});
                            currentPos += winheight;
                            ImGui::SetNextWindowSize({(float)live_width, (float)winheight});
                            module->drawUI(false);
                        }
                        processing::ui_call_list_mutex->unlock();
                        // ImGui::EndChild();
                    }
                    else
                    {
                        offline::render();
                    }

                    ImGui::EndTabItem();
                    main_ui_is_processing_selected = true;
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
#if 0
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
#endif
                if (ImGui::BeginTabItem("Settings"))
                {
                    settings::render();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Credits"))
                {
                    ImGui::BeginChild("credits_md", ImGui::GetContentRegionAvail(), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
                    credits_md.render();
                    ImGui::EndChild();
                    ImGui::EndTabItem();
                }
#ifdef ENABLE_DEBUG_MAP
                if (ImGui::BeginTabItem("Map"))
                {
                    tileMap tm;
                    ImGui::SetNextItemWidth(120);
                    ImGui::InputFloat("Latitude", &lat);
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(120);
                    ImGui::InputFloat("Longitude", &lon);
                    ImGui::SetNextItemWidth(120);
                    ImGui::InputFloat("Latitude##1", &lat1);
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(120);
                    ImGui::InputFloat("Longitude##1", &lon1);
                    ImGui::SetNextItemWidth(250);
                    ImGui::SliderInt("Zoom", &zoom, 0, 19);
                    if (ImGui::Button("Get tile from server"))
                    {
                        // mapTile tl(tm.downloadTile(tm.coorToTile({lat, lon}, zoom), zoom));
                        img = tm.getMapImage({lat, lon}, {lat1, lon1}, zoom);
                        ivw.update(img);
                    }
                    ivw.draw(ImVec2(800, 400));
                    ImGui::EndTabItem();
                }
#endif
            }
            ImGui::EndTabBar();
            ImGui::End();

            if (error::hasError)
                error::render();

            if (settings::show_imgui_demo)
                ImGui::ShowDemoWindow();
        }

        // Render toasts on top of everything, at the end of your code!
        // You should push style vars here
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.f);                                                    // Round borders
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(43.f / 255.f, 43.f / 255.f, 43.f / 255.f, 100.f / 255.f)); // Background color
        ImGui::RenderNotifications();                                                                              // <-- Here we render all notifications
        ImGui::PopStyleVar(1);                                                                                     // Don't forget to Pop()
        ImGui::PopStyleColor(1);
    }

    bool light_theme;
    bool status_bar;

    ctpl::thread_pool ui_thread_pool(8);
}
