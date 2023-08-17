#include <GLES3/gl3.h>
#include "core/style.h"
#include "imgui/imgui_flags.h"
#include "imgui/imgui.h"
#include "imgui_impl_android.h"
#include "imgui_impl_opengl3.h"
#include "loading_screen.h"

namespace satdump
{
    LoadingScreenSink::LoadingScreenSink(EGLDisplay *g_EglDisplay, EGLSurface *g_EglSurface) : g_EglDisplay{g_EglDisplay}, g_EglSurface{g_EglSurface}
    {
        draw_loader("Initializing");
    }

    LoadingScreenSink::~LoadingScreenSink()
    {
    }

    void LoadingScreenSink::receive(slog::LogMsg log)
    {
        if (log.lvl == slog::LOG_INFO)
            draw_loader(log.str);
    }

    void LoadingScreenSink::draw_loader(std::string str)
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplAndroid_NewFrame();
        ImGui::NewFrame();

        EGLint width, height;
        eglQuerySurface(*g_EglDisplay, *g_EglSurface, EGL_WIDTH, &width);
        eglQuerySurface(*g_EglDisplay, *g_EglSurface, EGL_HEIGHT, &height);

        ImGui::SetNextWindowPos({ 0, 0 });
        ImGui::SetNextWindowSize({(float)width, (float)height});
        ImGui::Begin("Loading Screen", nullptr, NOWINDOW_FLAGS | ImGuiWindowFlags_NoDecoration);
        ImGui::PushFont(style::bigFont);
        ImVec2 text_size = ImGui::CalcTextSize("SatDump");
        ImGui::SetCursorPos({((float)width / 2) - (text_size.x / 2), ((float)height / 2) - text_size.y});
        ImGui::TextUnformatted("SatDump");
        ImGui::PopFont();
        ImGui::SetCursorPos({ ((float)width / 2) - (ImGui::CalcTextSize(str.c_str()).x / 2), ((float)height / 2) + 10 });
        ImGui::TextUnformatted(str.c_str());
        ImGui::End();

        ImGui::Render();
        ImGuiIO &io = ImGui::GetIO();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        eglSwapBuffers(*g_EglDisplay, *g_EglSurface);
    }
}
