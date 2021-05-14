// ImGui SDL2 binding with OpenGL3
// In this binding, ImTextureID is used to store an OpenGL 'GLuint' texture identifier. Read the FAQ about ImTextureID in imgui.cpp.

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you use this binding you'll need to call 4 functions: ImGui_ImplXXXX_Init(), ImGui_ImplXXXX_NewFrame(), ImGui::Render() and ImGui_ImplXXXX_Shutdown().
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

#ifdef GL_PROFILE_GLES3

#ifndef IMGUI_IMPL_SDL_ES3
#define IMGUI_IMPL_SDL_ES3

struct SDL_Window;
typedef union SDL_Event SDL_Event;

IMGUI_API bool        ImGui_ImplSdlGLES3_Init(SDL_Window* window);
IMGUI_API void        ImGui_ImplSdlGLES3_Shutdown();
IMGUI_API void        ImGui_ImplSdlGLES3_NewFrame(SDL_Window* window);
IMGUI_API bool        ImGui_ImplSdlGLES3_ProcessEvent(SDL_Event* event);

// Use if you want to reset your rendering device without losing ImGui state.
IMGUI_API void        ImGui_ImplSdlGLES3_InvalidateDeviceObjects();
IMGUI_API bool        ImGui_ImplSdlGLES3_CreateDeviceObjects();
IMGUI_API void ImGui_ImplSdlGLES3_RenderDrawLists(ImDrawData* draw_data);

#endif // IMGUI_IMPL_SDL_ES3
#endif // GL_PROFILE_GLES3