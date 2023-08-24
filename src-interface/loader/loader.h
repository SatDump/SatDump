#pragma once
#ifdef __ANDROID__
#include <GLES3/gl3.h>
#else
#include <GLFW/glfw3.h>
#endif

namespace satdump
{
	void draw_loader(int width, int height, float scale, GLuint *image_texture, std::string str);
}