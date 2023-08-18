#pragma once

#include "logger.h"
#include <EGL/egl.h>
#include <GLES3/gl3.h>

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
        void push_frame(std::string str);
        EGLDisplay *g_EglDisplay;
        EGLSurface *g_EglSurface;
        GLuint image_texture;
    };
}
