#pragma once

#include "logger.h"
#include <thread>

namespace satdump
{
    class LoadingScreenSink : public slog::LoggerSink
    {
    public:
        LoadingScreenSink();
        void push_frame(std::string str);
        ~LoadingScreenSink();

    protected:
        void receive(slog::LogMsg log);

    private:
        intptr_t image_texture;
        bool loader_constant;
        std::string title;
        std::string slogan;

        std::thread::id thread_id;
    };
} // namespace satdump
