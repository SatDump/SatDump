#define SATDUMP_DLL_EXPORT2 1

#include "main_ui.h"
#include "common/audio/audio_sink.h"
#include "common/widgets/markdown_helper.h"
#include "core/backend.h"
#include "core/plugin.h"
#include "core/resources.h"
#include "imgui/imgui.h"
#include "imgui/imgui_flags.h"
#include "imgui_notify/imgui_notify.h"
#include "notify_logger_sink.h"
#include "offline.h"
#include "processing.h"
#include "satdump_vars.h"
#include "settings.h"
#include "status_logger_sink.h"

#include "imgui/implot/implot.h"
#include "imgui/implot3d/implot3d.h"

namespace satdump
{
    SATDUMP_DLL2 std::shared_ptr<RecorderApplication> recorder_app;
    SATDUMP_DLL2 std::shared_ptr<viewer::ViewerApplication> viewer_app2;
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

        recorder_app = std::make_shared<RecorderApplication>();
        viewer_app2 = std::make_shared<viewer::ViewerApplication>();
        open_recorder = satdump_cfg.main_cfg.contains("cli") && satdump_cfg.main_cfg["cli"].contains("start_recorder_device");

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
        // TODOREWORK SAVING AGAIN? viewer_app->save_settings();
        satdump_cfg.saveUser();
        recorder_app.reset();
        viewer_app2.reset();
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
                        for (std::shared_ptr<pipeline::ProcessingModule> module : *processing::ui_call_list)
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
                    viewer_app2->draw();
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
} // namespace satdump
