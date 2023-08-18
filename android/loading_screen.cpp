#include "common/image/image.h"
#include "imgui/imgui.h"
#include "imgui_impl_android.h"
#include "imgui_impl_opengl3.h"
#include "loading_screen.h"
#include "loader/loader.h"
#include "resources.h"

namespace satdump
{
    LoadingScreenSink::LoadingScreenSink(EGLDisplay *g_EglDisplay, EGLSurface *g_EglSurface) : g_EglDisplay{g_EglDisplay}, g_EglSurface{g_EglSurface}
    {
        image::Image<uint8_t> image;
        image.load_png(resources::getResourcePath("icon.png"));
        uint8_t *px = new uint8_t[image.width() * image.height() * 4];
        memset(px, 255, image.width() * image.height() * 4);

        if (image.channels() == 4)
        {
            for (int y = 0; y < (int)image.height(); y++)
                for (int x = 0; x < (int)image.width(); x++)
                    for (int c = 0; c < 4; c++)
                        px[image.width() * 4 * y + x * 4 + c] = image.channel(c)[image.width() * y + x];
        }
        else if (image.channels() == 3)
        {
            for (int y = 0; y < (int)image.height(); y++)
                for (int x = 0; x < (int)image.width(); x++)
                    for (int c = 0; c < 3; c++)
                        px[image.width() * 4 * y + x * 4 + c] = image.channel(c)[image.width() * y + x];
        }

        glGenTextures(1, &image_texture);
        glBindTexture(GL_TEXTURE_2D, image_texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width(), image.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
        image.clear();

        push_frame("Initializing");
    }

    LoadingScreenSink::~LoadingScreenSink()
    {
    }

    void LoadingScreenSink::receive(slog::LogMsg log)
    {
        if (log.lvl == slog::LOG_INFO)
            push_frame(log.str);
    }

    void LoadingScreenSink::push_frame(std::string str)
    {
        EGLint width, height;
        eglQuerySurface(*g_EglDisplay, *g_EglSurface, EGL_WIDTH, &width);
        eglQuerySurface(*g_EglDisplay, *g_EglSurface, EGL_HEIGHT, &height);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplAndroid_NewFrame();
        draw_loader(width, height, &image_texture, str);
        glViewport(0, 0, (int)width, (int)height);
        glClearColor(0.0666f, 0.0666f, 0.0666f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        eglSwapBuffers(*g_EglDisplay, *g_EglSurface);
    }
}
