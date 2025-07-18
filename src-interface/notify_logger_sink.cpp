#include "notify_logger_sink.h"
#include "imgui/imgui.h"
#include "imgui_notify/imgui_notify.h"
#include "logger.h"

namespace satdump
{
    NotifyLoggerSink::NotifyLoggerSink() {}

    NotifyLoggerSink::~NotifyLoggerSink() {}

    void NotifyLoggerSink::receive(slog::LogMsg log)
    {
        if (log.lvl == slog::LOG_WARN || log.lvl == slog::LOG_ERROR)
        {
            std::string title = log.lvl == slog::LOG_CRIT ? "Critical" : (log.lvl == slog::LOG_WARN ? "Warning" : "Error");
            ImGuiToastType type = log.lvl == slog::LOG_WARN ? ImGuiToastType_Error : (log.lvl == slog::LOG_WARN ? ImGuiToastType_Warning : ImGuiToastType_Error);
            notify_mutex.lock();

            for (size_t i = 0; i < ImGui::notifications.size(); i++)
            {
                if (strcmp(ImGui::notifications[i].content.c_str(), log.str.c_str()) == 0 && ImGui::notifications[i].type == type)
                {
                    int count = 0;
                    if (sscanf(ImGui::notifications[i].title.c_str(), std::string(title + " (%d)").c_str(), &count) != 1)
                        count = 1;
                    title += " (" + std::to_string(++count) + ")";
                    ImGui::notifications[i].title = title;
                    std::chrono::time_point<steady_clock> now = steady_clock::now();
                    if (now - ImGui::notifications[i].creation_time > NOTIFY_FADE_IN_OUT_TIME)
                        ImGui::notifications[i].creation_time = now - std::chrono::milliseconds(150);

                    notify_mutex.unlock();
                    return;
                }
            }

            ImGui::InsertNotification(ImGuiToast(type, title.c_str(), log.str.c_str()));
            notify_mutex.unlock();
        }
    }
} // namespace satdump