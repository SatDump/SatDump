#pragma once

#include <vector>
#include <iostream>
#include "logger.h"
#include "imgui/imgui.h"

namespace widgets
{
    class LoggerSinkWidget : public slog::LoggerSink
    {
    private:
        struct LogLine
        {
            slog::LogLevel lvl;
            std::string str;
        };

        std::vector<LogLine> all_lines;

    protected:
        void receive(slog::LogMsg log)
        {
            if (log.lvl >= sink_lvl)
                all_lines.push_back({log.lvl, format_log(log, false)});
        }

    public:
        void draw()
        {
            if (all_lines.size() > max_lines)
                all_lines.erase(all_lines.end() - max_lines, all_lines.end());

            for (LogLine &ll : all_lines)
            {
                std::string timestamp = ll.str.substr(0, 24);
                std::string text = ll.str.substr(24, ll.str.size());

                ImGui::Text("%s", timestamp.c_str());
                ImGui::SameLine();

                if (ll.lvl == slog::LOG_TRACE)
                    ImGui::TextColored(ImColor(255, 255, 255), "%s", text.c_str());
                else if (ll.lvl == slog::LOG_DEBUG)
                    ImGui::TextColored(ImColor(0, 255, 255), "%s", text.c_str());
                else if (ll.lvl == slog::LOG_INFO)
                    ImGui::TextColored(ImColor(0, 255, 0), "%s", text.c_str());
                else if (ll.lvl == slog::LOG_WARN)
                    ImGui::TextColored(ImColor(255, 255, 0), "%s", text.c_str());
                else if (ll.lvl == slog::LOG_ERROR)
                    ImGui::TextColored(ImColor(255, 0, 0), "%s", text.c_str());
                else if (ll.lvl == slog::LOG_CRIT)
                    ImGui::TextColored(ImColor(255, 0, 255), "%s", text.c_str());
            }
        }

        size_t max_lines = 1e3;
    };
}