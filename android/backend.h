#pragma once
#include "imgui/imgui.h"
#include "imgui_impl_android.h"
#include "imgui_impl_opengl3.h"
#include <stdint.h>
#include <utility>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <android_native_app_glue.h>

// Android-specific functions
static int HideSoftKeyboardInput();

// Standard bound functions
void funcSetMousePos(int, int);
std::pair<int, int> funcBeginFrame();
void funcEndFrame();
void funcSetIcon(uint8_t*);
void bindBackendFunctions();