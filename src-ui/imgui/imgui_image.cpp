#include "imgui/imgui_image.h"
#include "gl.h"

#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2ext.h>
#endif

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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void funcUpdateMMImageTexture(unsigned int gl_text, uint32_t* buffer, int width, int height)
{
    glBindTexture(GL_TEXTURE_2D, gl_text);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void funcDeleteImageTexture(unsigned int /*gl_text*/)
{
    //glDeleteTextures(1, &gl_text);
}

void bindImageTextureFunctions()
{
    makeImageTexture = funcMakeImageTexture;
    updateImageTexture = funcUpdateImageTexture;
    updateMMImageTexture = funcUpdateMMImageTexture;
    deleteImageTexture = funcDeleteImageTexture;
}
