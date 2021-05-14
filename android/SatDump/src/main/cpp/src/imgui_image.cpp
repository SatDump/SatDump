#include "imgui/imgui_image.h"
#include <SDL.h>
#include <GLES2/gl2.h>

unsigned int funcMakeImageTexture()
{
    GLuint gl_text;
    glGenTextures(1, &gl_text);
    return gl_text;
}

void funcUpdateImageTexture(unsigned int gl_text, uint32_t *buffer, int width, int height)
{
    glBindTexture(GL_TEXTURE_2D, gl_text);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(0x0CF2, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void funcDeleteImageTexture(unsigned int gl_text)
{
    //glDeleteTextures(1, &gl_text);
}

void bindImageTextureFunctions()
{
    makeImageTexture = funcMakeImageTexture;
    updateImageTexture = funcUpdateImageTexture;
    deleteImageTexture = funcDeleteImageTexture;
}