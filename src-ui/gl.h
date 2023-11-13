#include "gl3w/gl3w.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#else
#include "imgui/imgui_impl_opengl2.h"
#endif
#include <GLFW/glfw3.h> // Will drag system OpenGL headers