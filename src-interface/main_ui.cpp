#include "libs/base64/base64.h"
#include "utils/time.h"
#include <string>
#define SATDUMP_DLL_EXPORT2 1

#include "explorer/explorer.h"

#include "common/imgui_utils.h"
#include "core/style.h"
#include "recorder/recorder.h"

#include "status_logger_sink.h"

#include "common/audio/audio_sink.h"
#include "common/widgets/markdown_helper.h"
#include "common/widgets/menuitem_tooltip.h"
#include "core/backend.h"
#include "core/plugin.h"
#include "core/resources.h"
#include "imgui/imgui.h"
#include "imgui/imgui_flags.h"
#include "imgui_notify/imgui_notify.h"
#include "main_ui.h"
#include "notify_logger_sink.h"
#include "offline.h"
#include "satdump_vars.h"
#include "settings.h"

#include "imgui/implot/implot.h"
#include "imgui/implot3d/implot3d.h"

namespace satdump
{
    SATDUMP_DLL2 std::shared_ptr<explorer::ExplorerApplication> explorer_app;

    SATDUMP_DLL2 bool update_ui = true;

    widgets::MarkdownHelper credits_md;

    std::shared_ptr<NotifyLoggerSink> notify_logger_sink;
    std::shared_ptr<StatusLoggerSink> status_logger_sink;

    // Stuff
    std::string major_ui_constant;
    bool major_ui_constant2 = false;

    void showProcessing(); // TODOREWORK

    void initMainUI()
    {
        ImPlot::CreateContext();
        ImPlot3D::CreateContext();

        audio::registerSinks();
        offline::setup();
        settings::setup();

        // Register events
        eventBus->register_handler<ShowProcesingEvent>([](ShowProcesingEvent) { showProcessing(); });
        eventBus->register_handler<AddRecorderEvent>(
            [](AddRecorderEvent)
            {
                explorer_app->tryOpenSomethingInExplorer(
                    [](explorer::ExplorerApplication *)
                    {
                        logger->warn("Adding Recorder!");
                        eventBus->fire_event<explorer::ExplorerAddHandlerEvent>({std::make_shared<RecorderApplication>(), true}); // TODOREWORK do not bind this directly.
                    });
            });
        eventBus->register_handler<TryOpenFileInMainExplorerEvent>([](TryOpenFileInMainExplorerEvent e) { explorer_app->tryOpenFileInExplorer(e.path); });

        // Load credits MD
        std::ifstream ifs(resources::getResourcePath("credits.md"));
        std::string credits_markdown((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
        credits_md.set_md(credits_markdown);

        explorer_app = std::make_shared<explorer::ExplorerApplication>();

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

        // Important
        {
            const time_t timevalue = time(0);
            std::tm *timeConstant = gmtime(&timevalue);
            image::Image image;
            std::random_device dev;
            std::mt19937 rng(dev());
            std::uniform_int_distribution<std::mt19937::result_type> check(1, 1000);
            bool v;
            if (((timeConstant->tm_mon - 3) == 0 && (timeConstant->tm_mday - 1) == 0))
                v = check(rng) < 6.62607015e2;
            else
                v = ((timeConstant->tm_mday - 1) == 0) && check(rng) < (1.618033 * 2);

            if (v)
            {
                major_ui_constant = std::string({0x54, 0x72, 0x69, 0x61, 0x6c, 0x20, 0x45, 0x6e, 0x64, 0x73, 0x20, 0x3a, 0x20}) + timestamp_to_string(time(0) + check(rng) * 384000) + " ?";
                logger->error(major_ui_constant + " " + std::string({0x50, 0x6c, 0x65, 0x61, 0x73, 0x65, 0x20, 0x63, 0x68, 0x65, 0x63, 0x6b, 0x20,
                                                                     0x74, 0x68, 0x65, 0x20, 0x22, 0x3f, 0x22, 0x20, 0x6d, 0x65, 0x6e, 0x75, 0x21}));
            }
        }
    }

    void exitMainUI()
    {
        satdump_cfg.saveUser();
        explorer_app.reset();
    }

    // TODOREWORKUI
    bool offline_en = false;
    bool about_en = false;
    bool settings_en = false;

    void showProcessing() { offline_en = 1; }

    void renderMainUI()
    {
        if (update_ui)
        {
            style::setStyle();
            style::setFonts(ui_scale);
            update_ui = false;
        }

        std::pair<int, int> dims = backend::beginFrame();

        // On Windows minimizing makes the window dimensions 0,
        // which of course cause a whole lot of issues...
        // Simply abort any further rendering of this happens!
        if (dims.first == 0 && dims.second == 0)
        {
            backend::endFrame();
            return;
        }

        dims.second -= status_logger_sink->draw();

        {
            ImGui::SetNextWindowPos({0, 0});
            ImGui::SetNextWindowSize({(float)dims.first, (float)dims.second});
            ImGui::Begin("SatDump UI", nullptr, NOWINDOW_FLAGS | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoScrollWithMouse);

            if (ImGui::BeginMenuBar())
            {
                // Show/Hide explorer sidebar
                if (ImGui::MenuItem(u8"\uf85b", NULL, explorer_app->show_panel))
                    explorer_app->show_panel = !explorer_app->show_panel;

                if (ImGui::BeginMenu("File"))
                {
                    if (ImGui::MenuItem("Processing"))
                        offline_en = true;

                    if (ImGui::MenuItem("Settings"))
                        settings_en = true;

                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Add"))
                {
                    if (ImGui::MenuItem("Recorder"))
                        explorer_app->tryOpenSomethingInExplorer(
                            [](explorer::ExplorerApplication *)
                            {
                                eventBus->fire_event<explorer::ExplorerAddHandlerEvent>({std::make_shared<RecorderApplication>(), true}); // TODOREWORK do not bind this directly.
                            });

                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("?"))
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImU32(style::theme.orange));
                    if (major_ui_constant.size() && ImGui::MenuItem(major_ui_constant.c_str()))
                        major_ui_constant2 = true;
                    ImGui::PopStyleColor();

                    ImGui::EndMenu();
                }

                ImGui::EndMenuBar();
            }

            if (offline_en)
            {
                // ImGui::SetNextWindowSize({600 * ui_scale, 0});
                ImGui::SetNextWindowPos({0, 0});
                ImGui::SetNextWindowSize({(float)dims.first, (float)dims.second});
                if (ImGui::Begin("Start Processing", &offline_en, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse))
                {
                    offline::render();
                    ImGui::End();
                }
            }

            if (settings_en)
            {
                ImGui::OpenPopup("Settings");
                ImVec2 center = ImGui::GetIO().DisplaySize * 0.5f;
                ImGui::SetNextWindowPos({center.x, 50 * ui_scale}, ImGuiCond_Appearing, ImVec2(0.5f, 0.0f));
                ImGui::SetNextWindowSize({750 * ui_scale, 0});
            }

            if (ImGui::BeginPopupModal("Settings", &settings_en, ImGuiWindowFlags_AlwaysVerticalScrollbar))
            {
                settings::render();
                ImGui::EndPopup();
            }

            if (about_en)
            {
                ImGui::OpenPopup("About");
                ImVec2 center = ImGui::GetIO().DisplaySize * 0.5f;
                ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
                ImGui::SetNextWindowSize({750 * ui_scale, 0});
            }

            if (ImGui::BeginPopupModal("About", &about_en, ImGuiWindowFlags_AlwaysVerticalScrollbar))
            {
                credits_md.render();
                ImGui::Dummy({400 * ui_scale, 0});
                ImGui::EndPopup();
            }

            if (ImGui::BeginMenuBar())
            {
                if (widgets::BeginMenuTooltip("?", "Help"))
                {
                    if (ImGui::MenuItem("Documentation"))
                        ;
                    if (ImGui::MenuItem("About"))
                        about_en = true;
                    ImGui::EndMenu();
                }

                ImGui::EndMenuBar();
            }

            explorer_app->draw();

            ImGuiUtils_SendCurrentWindowToBack();
            ImGui::End();

            if (settings::show_imgui_demo)
            {
                ImGui::ShowDemoWindow();
                ImPlot::ShowDemoWindow();
                ImPlot3D::ShowDemoWindow();
            }
        }

        // Show stuff?
        if (major_ui_constant.size())
        {
            auto base64_to_str = [](std::string s) -> std::string
            {
                macaron::Base64 b;
                std::string o;
                b.Decode(s, o);
                return o;
            };

            if (major_ui_constant2 && ImGui::Begin(base64_to_str("Q09OQ0VSTlMgQUJPVVQgWU9VUiBTQVREVU1Qwq4gTElDRU5TRSBTVEFUVVM=").c_str(), &major_ui_constant2))
            {
                ImGui::Text("%s", base64_to_str("WW91IGFyZSBjdXJyZW50bHkgdXNpbmcgU2F0RHVtcChUTSkgdW5kZXIgdGhlIHRyaWFs").c_str());
                ImGui::Text("%s", base64_to_str("bGljZW5zZSwgZ3JhbnRlZCB0byBhbGwgdGhvc2Ugd2hvIHdvcnNoaXAgTk9BQS0xNSBhcHByb3ByaWF0ZWx5").c_str());
                ImGui::Text("%s", base64_to_str("Zm9yIGEgbGltaXRlZCwgcmFuZG9tIGFtb3VudCBvZiB0aW1lLg==").c_str());
                ImGui::Spacing();
                ImGui::Text("%s", base64_to_str("U3RhcnRpbmcgZnJvbSBET1kgOTEsIDIwMjYsIGEgY29tbWVyY2lhbCBsaWNlbnNlIHdpbGwgYmUgcmVxdWlyZWQgdG8gY29udGludWUg").c_str());
                ImGui::Text("%s", base64_to_str("dXNpbmcgU2F0RHVtcChUTSkuIFlvdSBtYXkgb2J0YWluIHN1Y2ggYSBsaWNlbnNlIGJ5IGNvbnRhY3RpbmcgU2F0RHVtcA==").c_str());
                ImGui::Text("%s", base64_to_str("SW50ZXJnYWxhY3RpYyBFbnRlcnByaXNlcyBieSBzZW5kaW5nIGEgbWVzc2FnZSB1c2luZyB0aGUgZm9sbG93aW5nIHNwZWNpZmljYXRpb25zCg==").c_str());
                ImGui::Text("%s", base64_to_str("dG8gdGhlIG1haW4gU2F0RHVtcCBSZWxheSBTYXRlbGxpdGUgKG11c3QgYmUgc2VudCB2aWEgZ3Jhdml0YXRpb25hbCB3YXZlcywK").c_str());
                ImGui::Text("%s", base64_to_str("YXQgeW91ciBvd24gcmlzaykgOg==").c_str());
                ImGui::Text("%s", base64_to_str("IC0gQVNDSUkgaW4gQVBJRCAxMDQ=").c_str());
                ImGui::Text("%s", base64_to_str("IC0gQ29udiAxLzI=").c_str());
                ImGui::Text("%s", base64_to_str("IC0gUlMgMjIzLzI1NSBJPTg=").c_str());
                ImGui::Text("%s", base64_to_str("IC0gUHJlLWNvZGVkIEdNU0sgKHNlZSBDQ1NEUyA0MDEgZm9yIGRldGFpbHMp").c_str());
                ImGui::End();
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
