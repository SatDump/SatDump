#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include "loading_screen.h"
#include "loader/loader.h"
#include "core/style.h"

namespace satdump
{
    LoadingScreenSink::LoadingScreenSink(GLFWwindow* window, float scale, GLFWimage* img) : window{window},
                                                                                            scale{scale}
    {
        macos_scale = style::macos_framebuffer_scale();

        glGenTextures(1, &image_texture);
        glBindTexture(GL_TEXTURE_2D, image_texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img->width, img->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, img->pixels);

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
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();

        draw_loader(display_w / macos_scale, display_h / macos_scale, scale, &image_texture, str);

        glViewport(0, 0, display_w, display_h);
        glClearColor(0.0666f, 0.0666f, 0.0666f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}
