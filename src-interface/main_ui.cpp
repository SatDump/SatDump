#define SATDUMP_DLL_EXPORT2 1
#include "main_ui.h"
#include "imgui/imgui_flags.h"
#include "imgui/imgui.h"
#include "processing.h"
#include "offline.h"
#include "settings.h"
#include "satdump_vars.h"
#include "core/backend.h"
#include "resources.h"
#include "common/widgets/markdown_helper.h"
#include "common/audio/audio_sink.h"
#include "imgui_notify/imgui_notify.h"
#include "notify_logger_sink.h"
#include "status_logger_sink.h"

#include "imgui/implot/implot.h"
#include "imgui/implot3d/implot3d.h"

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
    SATDUMP_DLL2 std::shared_ptr<RecorderApplication> recorder_app;
    SATDUMP_DLL2 std::shared_ptr<ViewerApplication> viewer_app;
    std::vector<std::shared_ptr<Application>> other_apps;

    SATDUMP_DLL2 bool update_ui = true;
    bool open_recorder;

    widgets::MarkdownHelper credits_md;

    std::shared_ptr<NotifyLoggerSink> notify_logger_sink;
    std::shared_ptr<StatusLoggerSink> status_logger_sink;

    void initMainUI()
    {
        ImPlot::CreateContext();
        ImPlot3D::CreateContext();

        audio::registerSinks();
        offline::setup();
        settings::setup();

        // Load credits MD
        std::ifstream ifs(resources::getResourcePath("credits.md"));
        std::string credits_markdown((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
        credits_md.set_md(credits_markdown);

        registerViewerHandlers();

        recorder_app = std::make_shared<RecorderApplication>();
        viewer_app = std::make_shared<ViewerApplication>();
        open_recorder = satdump::config::main_cfg.contains("cli") && satdump::config::main_cfg["cli"].contains("start_recorder_device");

        eventBus->fire_event<AddGUIApplicationEvent>({other_apps});

        // Logger status bar sync
        status_logger_sink = std::make_shared<StatusLoggerSink>();
        if (status_logger_sink->is_shown())
            logger->add_sink(status_logger_sink);

        // Shut down the logger init buffer manually to prevent init warnings
        // From showing as a toast, or in the product processor screen
        completeLoggerInit();

        // Logger notify sink
        notify_logger_sink = std::make_shared<NotifyLoggerSink>();
        logger->add_sink(notify_logger_sink);
    }

    void exitMainUI()
    {
        recorder_app->save_settings();
        viewer_app->save_settings();
        config::saveUserConfig();
        recorder_app.reset();
        viewer_app.reset();
    }

    void renderMainUI()
    {
        if (update_ui)
        {
            style::setStyle();
            style::setFonts(ui_scale);
            update_ui = false;
        }

        std::pair<int, int> dims = backend::beginFrame();
        dims.second -= status_logger_sink->draw();

        // else
        {
            ImGui::SetNextWindowPos({0, 0});
            ImGui::SetNextWindowSize({(float)dims.first, (float)dims.second});
            ImGui::Begin("SatDump UI", nullptr, NOWINDOW_FLAGS | ImGuiWindowFlags_NoDecoration);
            if (ImGui::BeginTabBar("Main TabBar", ImGuiTabBarFlags_None))
            {
                if (ImGui::BeginTabItem("Offline processing"))
                {
                    if (processing::is_processing)
                    {
                        // ImGui::BeginChild("OfflineProcessingChild");
                        processing::ui_call_list_mutex->lock();
                        int live_width = dims.first; // ImGui::GetWindowWidth();
                        int live_height = /*ImGui::GetWindowHeight()*/ dims.second - ImGui::GetCursorPos().y;
                        float winheight = processing::ui_call_list->size() > 0 ? live_height / processing::ui_call_list->size() : live_height;
                        float currentPos = ImGui::GetCursorPos().y;
                        ImGui::PushStyleColor(ImGuiCol_TitleBg, ImGui::GetStyleColorVec4(ImGuiCol_TitleBgActive));
                        for (std::shared_ptr<ProcessingModule> module : *processing::ui_call_list)
                        {
                            ImGui::SetNextWindowPos({0, currentPos});
                            currentPos += winheight;
                            ImGui::SetNextWindowSize({(float)live_width, (float)winheight});
                            module->drawUI(false);
                        }
                        ImGui::PopStyleColor();
                        processing::ui_call_list_mutex->unlock();
                        // ImGui::EndChild();
                    }
                    else
                    {
                        ImGui::BeginChild("offlineprocessing", ImGui::GetContentRegionAvail());
                        offline::render();
                        ImGui::EndChild();
                    }

                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Recorder", nullptr, open_recorder ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None))
                {
                    open_recorder = false;
                    recorder_app->draw();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Viewer"))
                {
                    viewer_app->draw();
                    ImGui::EndTabItem();
                }
                for (auto &app : other_apps)
                {
                    if (ImGui::BeginTabItem(std::string(app->get_name() + "##appsoption").c_str()))
                    {
                        app->draw();
                        ImGui::EndTabItem();
                    }
                }
                if (ImGui::BeginTabItem("Settings"))
                {
                    ImGui::BeginChild("settings", ImGui::GetContentRegionAvail());
                    settings::render();
                    ImGui::EndChild();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("About"))
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
            ImGuiUtils_SendCurrentWindowToBack();
            ImGui::End();

            if (settings::show_imgui_demo)
            {
                ImGui::ShowDemoWindow();
                ImPlot::ShowDemoWindow();
                ImPlot3D::ShowDemoWindow();
            }
        }

        // Render toasts on top of everything, at the end of your code!
        // You should push style vars here
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, (ImVec4)style::theme.notification_bg);
        notify_logger_sink->notify_mutex.lock();
        ImGui::RenderNotifications();
        notify_logger_sink->notify_mutex.unlock();
        ImGui::PopStyleVar(1);
        ImGui::PopStyleColor(1);

        backend::endFrame();
    }

    SATDUMP_DLL2 ctpl::thread_pool ui_thread_pool(8);
}
