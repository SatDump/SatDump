#pragma once
#include "imgui/imgui.h"
#include "imgui_impl_android.h"
#include "imgui_impl_opengl3.h"
#include <stdint.h>
#include <utility>
#include <EGL/egl.h>
#include <GLES3/gl3.h>

float funcDeviceScale();
void funcRebuildFonts();
void funcSetMousePos(int, int);
std::pair<int, int> funcBeginFrame();
void funcEndFrame();
void funcSetIcon(uint8_t*, int, int);
void bindBackendFunctions();
