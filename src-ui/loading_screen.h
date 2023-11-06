#pragma once

#include <GLFW/glfw3.h>
#include "logger.h"

namespace satdump
{
    class LoadingScreenSink : public slog::LoggerSink
    {
    public:
        LoadingScreenSink(GLFWwindow* window, float scale, GLFWimage* img, bool fallback_gl);
        ~LoadingScreenSink();
    protected:
        void receive(slog::LogMsg log);
    private:
        void push_frame(std::string str);
        GLFWwindow* window;
        float scale, macos_scale;
        GLuint image_texture;
        bool fallback_gl;
    };
}