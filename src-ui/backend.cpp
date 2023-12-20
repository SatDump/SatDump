#include <GLFW/glfw3.h>
#include "core/backend.h"
#include "backend.h"

extern GLFWwindow* window;

void funcSetMousePos(int x, int y)
{
	glfwSetCursorPos(window, x, y);
	ImGui_ImplGlfw_CursorPosCallback(window, x, y);
}

void bindBackendFunctions()
{
	backend::setMousePos = funcSetMousePos;
}