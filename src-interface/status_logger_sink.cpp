#include "status_logger_sink.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "core/config.h"

namespace satdump
{
    StatusLoggerSink::StatusLoggerSink()
    {
        str = "Welcome to SatDump!";
        lvl = "";
        show = config::main_cfg["user_interface"]["status_bar"]["value"].get<bool>();
    }

    StatusLoggerSink::~StatusLoggerSink()
    {
    }

    bool StatusLoggerSink::is_shown()
    {
        return show;
    }

    void StatusLoggerSink::receive(slog::LogMsg log)
    {
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
        int height = 0;
        if (ImGui::BeginViewportSideBar("##MainStatusBar", ImGui::GetMainViewport(), ImGuiDir_Down, ImGui::GetFrameHeight(),
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar)) {
            if (ImGui::BeginMenuBar()) {
                ImGui::TextUnformatted(lvl.c_str());
                ImGui::SameLine(75);
                ImGui::Separator();
                ImGui::TextDisabled("%s", str.c_str());
                height = ImGui::GetWindowHeight();
                ImGui::EndMenuBar();
            }
            ImGui::End();
        }

        return height;
    }
}