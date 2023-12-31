#pragma once
#include "logger.h"

namespace satdump
{
    class LoadingScreenSink : public slog::LoggerSink
    {
    public:
        LoadingScreenSink(float scale);
        void push_frame(std::string str);
        ~LoadingScreenSink();
    protected:
        void receive(slog::LogMsg log);
    private:
        float scale;
        unsigned int image_texture;
    };
}
