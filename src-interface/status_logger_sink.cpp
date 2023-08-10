#include "status_logger_sink.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

namespace satdump
{
    StatusLoggerSink::StatusLoggerSink()
    {
        str = "";
        lvl = "Welcome to SatDump!";
    }

    StatusLoggerSink::~StatusLoggerSink()
    {
    }

    void StatusLoggerSink::receive(slog::LogMsg log)
    {
        if (log.lvl >= slog::LOG_INFO)
        {
            if (log.lvl == slog::LOG_INFO)
                lvl = "Info:";
            else if (log.lvl == slog::LOG_WARN)
                lvl = "Warning:";
            else if (log.lvl == slog::LOG_ERROR)
                lvl = "ERROR:";
            else
                lvl = "CRITICAL:";

            str = log.str;
        }
    }

    int StatusLoggerSink::draw()
    {
        int height = 0;
        if (ImGui::BeginViewportSideBar("##MainStatusBar", ImGui::GetMainViewport(), ImGuiDir_Down, ImGui::GetFrameHeight(),
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar)) {
            if (ImGui::BeginMenuBar()) {
                ImGui::Text(lvl.c_str());
                ImGui::SameLine();
                ImGui::TextDisabled(str.c_str());
                height = ImGui::GetWindowHeight();
                ImGui::EndMenuBar();
            }
            ImGui::End();
        }

        return height;
    }
}