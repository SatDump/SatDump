#pragma once

#include <vector>
#include <iostream>
#include "spdlog/sinks/base_sink.h"
#include "imgui/imgui.h"

namespace widgets
{
    template <typename Mutex>
    class LoggerSinkWidget : public spdlog::sinks::base_sink<Mutex>
    {
    private:
        struct LogLine
        {
            spdlog::level::level_enum lvl;
            std::string str;
        };

        std::vector<LogLine> all_lines;

    protected:
        void sink_it_(const spdlog::details::log_msg &msg) override
        {
            spdlog::memory_buf_t formatted;
            spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
            all_lines.push_back({msg.level, fmt::to_string(formatted)});
        }

        void flush_() override
        {
        }

    public:
        void draw()
        {
            if (all_lines.size() > max_lines)
                all_lines.erase(all_lines.end() - max_lines, all_lines.end());

            for (LogLine &ll : all_lines)
            {
                std::string timestamp = ll.str.substr(0, 22);
                std::string text = ll.str.substr(22, ll.str.size());

                ImGui::Text("%s", timestamp.c_str());
                ImGui::SameLine();

                if (ll.lvl == spdlog::level::trace)
                    ImGui::TextColored(ImColor(255, 255, 255), "%s", text.c_str());
                else if (ll.lvl == spdlog::level::debug)
                    ImGui::TextColored(ImColor(0, 255, 255), "%s", text.c_str());
                else if (ll.lvl == spdlog::level::info)
                    ImGui::TextColored(ImColor(0, 255, 0), "%s", text.c_str());
                else if (ll.lvl == spdlog::level::warn)
                    ImGui::TextColored(ImColor(255, 255, 0), "%s", text.c_str());
                else if (ll.lvl == spdlog::level::err)
                    ImGui::TextColored(ImColor(255, 0, 0), "%s", text.c_str());
                else if (ll.lvl == spdlog::level::critical)
                    ImGui::TextColored(ImColor(255, 0, 255), "%s", text.c_str());
            }
        }

        size_t max_lines = 1e3;
    };
}