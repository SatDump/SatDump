#pragma once
#include <stdint.h>
#include <utility>
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_opengl3.h"
#include "gl.h"

void funcSetMousePos(int x, int y);
std::pair<int, int> funcBeginFrame();
void funcEndFrame();
void funcSetIcon(uint8_t *image, int w, int h);
void bindBackendFunctions();
