#pragma once

#include "logger.h"
#include <EGL/egl.h>

namespace satdump
{
    class LoadingScreenSink : public slog::LoggerSink
    {
    public:
        LoadingScreenSink(EGLDisplay *g_EglDisplay, EGLSurface *g_EglSurface);
        ~LoadingScreenSink();
    protected:
        void receive(slog::LogMsg log);
    private:
        void draw_loader(std::string str);
        EGLDisplay *g_EglDisplay;
        EGLSurface *g_EglSurface;
    };
}
