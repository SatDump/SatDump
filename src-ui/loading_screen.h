#pragma once

#include <GLFW/glfw3.h>
#include "logger.h"

namespace satdump
{
    class LoadingScreenSink : public slog::LoggerSink
    {
    public:
        LoadingScreenSink(GLFWwindow* window, GLFWimage* img);
        ~LoadingScreenSink();
    protected:
        void receive(slog::LogMsg log);
    private:
        void draw_loader(std::string str);
        GLFWwindow* window;
        GLuint image_texture;
    };
}