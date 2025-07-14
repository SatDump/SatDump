#include "status_logger_sink.h"
#include "common/imgui_utils.h"
#include "core/config.h"
#include "core/plugin.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "main_ui.h"
#include "utils/time.h"

SATDUMP_DLL extern float ui_scale;

namespace satdump
{
    StatusLoggerSink::StatusLoggerSink()
    {
        show_bar = satdump_cfg.main_cfg["user_interface"]["status_bar"]["value"].get<bool>();
        show_log = false;

        eventBus->register_handler<SetIsProcessingEvent>([this](const SetIsProcessingEvent &) { processing_tasks_n++; });
        eventBus->register_handler<SetIsDoneProcessingEvent>(
            [this](const SetIsDoneProcessingEvent &)
            {
                processing_tasks_n--;
                if (processing_tasks_n < 0)
                    processing_tasks_n = 0;
            });
    }

    StatusLoggerSink::~StatusLoggerSink() {}

    bool StatusLoggerSink::is_shown() { return show_bar; }

    void StatusLoggerSink::receive(slog::LogMsg log)
    {
        widgets::LoggerSinkWidget::receive(log);
        if (log.lvl >= slog::LOG_INFO)
        {
            if (log.lvl == slog::LOG_INFO)
                lvl = "Info";
            else if (log.lvl == slog::LOG_WARN)
                lvl = "Warning";
            else if (log.lvl == slog::LOG_ERROR)
                lvl = "Error";
            else if (log.lvl == slog::LOG_CRIT)
                lvl = "Critical";
            else
                lvl = "";

            str = log.str;
        }
    }

    int StatusLoggerSink::draw()
    {
        // Check if status bar should be drawn
        if (!show_bar)
            return 0;

        // if (processing::is_processing && ImGuiUtils_OfflineProcessingSelected())
        //     for (std::shared_ptr<pipeline::ProcessingModule> module : *processing::ui_call_list)
        //         if (module->getIDM() == "products_processor")
        //             return 0; TODOREWORK

        // Draw status bar
        int height = 0;
        if (ImGui::BeginViewportSideBar("##MainStatusBar", ImGui::GetMainViewport(), ImGuiDir_Down, ImGui::GetFrameHeight(),
                                        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoNavFocus))
        {
            if (ImGui::BeginMenuBar())
            {
                ImGui::TextUnformatted(lvl.c_str());
                ImGui::SameLine(75 * ui_scale);
                ImGui::Separator();
                ImGui::TextDisabled("%s", str.c_str());
                if (ImGui::IsItemClicked())
                    show_log = true;

                height = ImGui::GetWindowHeight();

                ImGui::SetCursorPosX(ImGui::GetWindowSize().x * 0.75);
                ImGui::Separator();

                ImGui::EndMenuBar();
            }

            // TODOREWORK
            if (processing_tasks_n > 0)
            {
                size_t pos = getTime() * 200;
                size_t offset = ImGui::GetWindowSize().x * 0.75 - 200 * ui_scale;
                auto p1 = ImGui::GetWindowPos();
                p1.x += offset + pos % (size_t)(ImGui::GetWindowSize().x * 0.25 + 200 * ui_scale);
                for (int i = 0; i < 10; i++)
                {
                    int xpos1 = p1.x + (i * 6) * ui_scale;
                    int xpos2 = p1.x + (i * 6 + 3) * ui_scale;
                    if (xpos1 >= ImGui::GetWindowSize().x * 0.75)
                        ImGui::GetForegroundDrawList()->AddRectFilled(ImVec2(xpos1, p1.y), ImVec2(xpos2, p1.y + height), ImGui::GetColorU32(ImGuiCol_CheckMark));
                }
            }

            ImGui::End();
        }

        if (show_log)
        {
            static ImVec2 last_size;
            ImVec2 display_size = ImGui::GetIO().DisplaySize;
            bool did_resize = display_size.x != last_size.x || display_size.y != last_size.y;
            ImGui::SetNextWindowSize(ImVec2(display_size.x, (display_size.y * 0.3) - height), did_resize ? ImGuiCond_Always : ImGuiCond_Appearing);
            ImGui::SetNextWindowPos(ImVec2(0, display_size.y * 0.7), did_resize ? ImGuiCond_Always : ImGuiCond_Appearing, ImVec2(0, 0));
            last_size = display_size;

            ImGui::SetNextWindowBgAlpha(1.0);
            ImGui::Begin("SatDump Log", &show_log, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse);
            widgets::LoggerSinkWidget::draw();

            ImGui::End();
        }

        return height;
    }
} // namespace satdump