#pragma once

#include "core/style.h"
#include "imgui/imgui.h"
#include <functional>
#include <string>
#include <thread>

namespace satdump
{
    namespace utils
    {
        class TaskedButton
        {
        private:
            std::thread the_thread;
            bool is_running = false;

        public:
            TaskedButton() {}
            ~TaskedButton()
            {
                if (the_thread.joinable())
                    the_thread.join();
            }

            void draw(const char *title, std::function<void()> func)
            {
                bool isr = is_running;
                if (isr)
                    style::beginDisabled();

                if (ImGui::Button(title))
                {
                    if (the_thread.joinable())
                        the_thread.join();
                    the_thread = std::thread(
                        [this, func]()
                        {
                            is_running = true;
                            func();
                            is_running = false;
                        });
                }

                if (isr)
                    style::endDisabled();
            }
        };
    } // namespace utils
} // namespace satdump