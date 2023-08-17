#include "core/style.h"
#include "imgui/imgui_flags.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include "loading_screen.h"

namespace satdump
{
    LoadingScreenSink::LoadingScreenSink(GLFWwindow* window, GLFWimage* img) : window{window}
    {
        glGenTextures(1, &image_texture);
        glBindTexture(GL_TEXTURE_2D, image_texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img->width, img->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, img->pixels);

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
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos({ 0, 0 });
        ImGui::SetNextWindowSize({(float)display_w, (float)display_h});
        ImGui::Begin("Loading Screen", nullptr, NOWINDOW_FLAGS | ImGuiWindowFlags_NoDecoration);
        ImVec2 reference_pos = { (float)display_w * 0.2f, ((float)display_h * 0.5f) - 125 };
        ImGui::SetCursorPos(reference_pos);
        ImGui::Image((void*)(intptr_t)image_texture, ImVec2(225, 225));
        ImGui::SetCursorPos({ reference_pos.x + 260, reference_pos.y + 70 });
        ImGui::PushFont(style::bigFont);
        ImGui::TextUnformatted("SatDump");
        ImGui::PopFont();
        ImGui::SetCursorPos({ reference_pos.x + 260, reference_pos.y + 125 });
        ImGui::TextUnformatted(str.c_str());
        ImGui::End();

        ImGui::Render();
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.0666f, 0.0666f, 0.0666f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }
}
