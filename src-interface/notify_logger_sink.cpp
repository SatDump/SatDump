#include "notify_logger_sink.h"
#include "i18n.h"
#include "imgui/imgui.h"
#include "imgui_notify/imgui_notify.h"
#include "logger.h"

namespace satdump
{
    NotifyLoggerSink::NotifyLoggerSink() {}

    NotifyLoggerSink::~NotifyLoggerSink() {}

    void NotifyLoggerSink::receive(slog::LogMsg log)
    {
        if (log.lvl == slog::LOG_NOTICE || log.lvl == slog::LOG_WARN || log.lvl == slog::LOG_ERROR)
        {
            std::string title;
            ImGuiToastType type;
            switch (log.lvl)
            {
            case slog::LOG_NOTICE:
            {
                title = _("Notice");
                type = ImGuiToastType_Info;
                break;
            }
            case slog::LOG_WARN:
            {
                title = _("Warning");
                type = ImGuiToastType_Warning;
                break;
            }
            default:
            {
                title = _("Error");
                type = ImGuiToastType_Error;
            }
            }

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