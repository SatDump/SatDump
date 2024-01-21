#include "imgui/imgui.h"
#include "core/style.h"
#include "logger_sink.h"

namespace widgets
{
    void LoggerSinkWidget::receive(slog::LogMsg log)
    {
        if (log.lvl >= sink_lvl)
        {
            mtx.lock();
            new_item = true;
            all_lines.push_back({ log.lvl, format_log(log, false) });
            if (all_lines.size() == max_lines)
                all_lines.pop_front();
            mtx.unlock();
        }
    }

    void LoggerSinkWidget::draw()
    {
        mtx.lock();
        for (LogLine& ll : all_lines)
        {
            std::string timestamp = ll.str.substr(0, 24);
            std::string text = ll.str.substr(24, ll.str.size());

            ImGui::Text("%s", timestamp.c_str());
            ImGui::SameLine();

            if (ll.lvl == slog::LOG_TRACE)
                ImGui::TextUnformatted(text.c_str());
            else if (ll.lvl == slog::LOG_DEBUG)
                ImGui::TextColored(style::theme.cyan, "%s", text.c_str());
            else if (ll.lvl == slog::LOG_INFO)
                ImGui::TextColored(style::theme.green, "%s", text.c_str());
            else if (ll.lvl == slog::LOG_WARN)
                ImGui::TextColored(style::theme.yellow, "%s", text.c_str());
            else if (ll.lvl == slog::LOG_ERROR)
                ImGui::TextColored(style::theme.red, "%s", text.c_str());
            else if (ll.lvl == slog::LOG_CRIT)
                ImGui::TextColored(style::theme.fuchsia, "%s", text.c_str());
        }
        if (new_item)
        {
            ImGui::SetScrollHereY(1.0f);
            new_item = false;
        }
        if(ImGui::IsWindowAppearing())
            new_item = true;
        mtx.unlock();
    }
}