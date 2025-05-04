#include "backend.h"
#include "core/backend.h"
#include "core/style.h"
#include <GLFW/glfw3.h>
#include <mutex>
#include <vector>

extern GLFWwindow *window;
extern bool fallback_gl;

std::mutex glfw_frame_mtx;

float funcDeviceScale()
{
    float display_scale;
#if GLFW_VERSION_MAJOR > 3 || (GLFW_VERSION_MAJOR == 3 && GLFW_VERSION_MINOR >= 3)
    glfwGetWindowContentScale(window, &display_scale, nullptr);
    display_scale /= style::macos_framebuffer_scale();
#else
    display_scale = 1.0f;
#endif
    return display_scale;
}

void funcRebuildFonts()
{
#ifndef IMGUI_IMPL_OPENGL_ES2
    if (fallback_gl)
    {
        ImGui_ImplOpenGL2_DestroyFontsTexture();
        ImGui_ImplOpenGL2_CreateFontsTexture();
    }
    else
#endif
    {
        ImGui_ImplOpenGL3_DestroyFontsTexture();
        ImGui_ImplOpenGL3_CreateFontsTexture();
    }
}

void funcSetMousePos(int x, int y)
{
    glfwSetCursorPos(window, x, y);
    ImGui_ImplGlfw_CursorPosCallback(window, x, y);
}

std::pair<int, int> funcBeginFrame()
{
    glfw_frame_mtx.lock();
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
    glfw_frame_mtx.unlock();
    return {wwidth, wheight};
}

void funcEndFrame()
{
    glfw_frame_mtx.lock();
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(style::theme.frame_bg.Value.x, style::theme.frame_bg.Value.y, style::theme.frame_bg.Value.z, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

#ifndef IMGUI_IMPL_OPENGL_ES2
    if (fallback_gl)
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
    else
#endif
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
    glfwPollEvents();
    glfw_frame_mtx.unlock();
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

//

#include "nfd/include/nfd.hpp"
#include "nfd/include/nfd_glfw3.h"

std::string selectFolderDialog(std::string default_path)
{
    // Init file dialogs
    NFD::Guard nfdGuard;

    glfw_frame_mtx.lock();

    NFD::UniquePath outPath;

    // show the dialog
    nfdwindowhandle_t h;
    NFD_GetNativeWindowFromGLFWWindow(window, &h);

    glfw_frame_mtx.unlock();

    nfdresult_t result = NFD::PickFolder(outPath, default_path == "" ? nullptr : default_path.c_str(), h);

    if (result == NFD_OKAY)
        return outPath.get();
    else if (result == NFD_CANCEL)
        return ""; //"User pressed cancel.";
    else
        return ""; // "Error: " + std::string(NFD::GetError());
}

std::string selectFileDialog(std::vector<std::pair<std::string, std::string>> filters, std::string default_path)
{
    // Init file dialogs
    NFD::Guard nfdGuard;

    glfw_frame_mtx.lock();

    NFD::UniquePath outPath;
    // show the dialog
    nfdwindowhandle_t h;
    NFD_GetNativeWindowFromGLFWWindow(window, &h);

    glfw_frame_mtx.unlock();

    std::vector<nfdfilteritem_t> filt;
    for (auto &f : filters)
        filt.push_back({f.first.c_str(), f.second.c_str()});

    nfdresult_t result = NFD::OpenDialog(outPath, filt.data(), filt.size(), default_path == "" ? nullptr : default_path.c_str(), h);

    if (result == NFD_OKAY)
        return outPath.get();
    else if (result == NFD_CANCEL)
        return ""; //"User pressed cancel.";
    else
        return ""; // "Error: " + std::string(NFD::GetError());
}

std::string saveFileDialog(std::vector<std::pair<std::string, std::string>> filters, std::string default_path, std::string default_name)
{
    // Init file dialogs
    NFD::Guard nfdGuard;

    glfw_frame_mtx.lock();

    NFD::UniquePath outPath;
    // show the dialog
    nfdwindowhandle_t h;
    NFD_GetNativeWindowFromGLFWWindow(window, &h);

    glfw_frame_mtx.unlock();

    std::vector<nfdfilteritem_t> filt;
    for (auto &f : filters)
        filt.push_back({f.first.c_str(), f.second.c_str()});

    nfdresult_t result = NFD::SaveDialog(outPath, filt.data(), filt.size(), default_path.c_str(), default_name.c_str(), h);

    if (result == NFD_OKAY)
        return outPath.get();
    else if (result == NFD_CANCEL)
        return ""; //"User pressed cancel.";
    else
        return ""; // "Error: " + std::string(NFD::GetError());
}

void bindBackendFunctions()
{
    backend::device_scale = funcDeviceScale();

    backend::rebuildFonts = funcRebuildFonts;
    backend::setMousePos = funcSetMousePos;
    backend::beginFrame = funcBeginFrame;
    backend::endFrame = funcEndFrame;
    backend::setIcon = funcSetIcon;

    backend::selectFolderDialog = selectFolderDialog;
    backend::selectFileDialog = selectFileDialog;
    backend::saveFileDialog = saveFileDialog;
}
