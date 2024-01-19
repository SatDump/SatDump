#include <GLFW/glfw3.h>
#include "core/style.h"
#include "core/backend.h"
#include "backend.h"

extern GLFWwindow* window;
extern bool fallback_gl;

void funcSetMousePos(int x, int y)
{
	glfwSetCursorPos(window, x, y);
	ImGui_ImplGlfw_CursorPosCallback(window, x, y);
}

std::pair<int, int> funcBeginFrame()
{
    // Start the Dear ImGui frame
#ifndef IMGUI_IMPL_OPENGL_ES2
    if (fallback_gl)
        ImGui_ImplOpenGL2_NewFrame();
    else
#endif
        ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    int wwidth, wheight;
    glfwGetWindowSize(window, &wwidth, &wheight);
    return {wwidth, wheight};
}

void funcEndFrame()
{
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(IMCOLOR_WINDOWBG.Value.x, IMCOLOR_WINDOWBG.Value.y, IMCOLOR_WINDOWBG.Value.z, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

#ifndef IMGUI_IMPL_OPENGL_ES2
    if (fallback_gl)
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
    else
#endif
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
    glfwPollEvents();
}

void funcSetIcon(uint8_t *image, int w, int h)
{
#ifndef _WIN32
    GLFWimage img;
    img.pixels = image;
    img.width = w;
    img.height = h;
    glfwSetWindowIcon(window, 1, &img);
#endif
}

void bindBackendFunctions()
{
    backend::setMousePos = funcSetMousePos;
    backend::beginFrame = funcBeginFrame;
    backend::endFrame = funcEndFrame;
    backend::setIcon = funcSetIcon;
}
