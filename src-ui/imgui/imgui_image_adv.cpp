// SatDump OpenGL 3 texture functions
// - Functions in this file will load through gl3w and must be complaint with OpenGL 3+
// - Functions here should override functions already set up in imgui_image_base.cpp
//   to provide mordern/advanced functionality

#include "imgui/imgui_image.h"
#include "gl3w/gl3w.h"
#include "logger.h"

void funcUpdateMMImageTexture_GL3(unsigned int gl_text, uint32_t* buffer, int width, int height)
{
    glBindTexture(GL_TEXTURE_2D, gl_text);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void bindAdvancedTextureFunctions()
{
    if (gl3wInit())
    {
        logger->critical("Could not init gl3w! Exiting");
        exit(1);
    }
    updateMMImageTexture = funcUpdateMMImageTexture_GL3;
}
