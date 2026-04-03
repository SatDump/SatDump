#pragma once
#include "imgui/imgui.h"
#include <chrono>
#include <string>

namespace satdump
{
    namespace widgets
    {
        class TimedMessage
        {
        private:
            ImColor color;
            std::chrono::time_point<std::chrono::steady_clock> *start_time;
            std::string message;

        public:
            TimedMessage();
            ~TimedMessage();
            void set_message(ImColor color, std::string message);
            void draw();
        };
    } // namespace widgets
} // namespace satdump