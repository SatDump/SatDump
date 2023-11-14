// SatDump base OpenGL texture functions
// - All functions specified in imgui/imgui_image.h should be defined/assigned here
// - Only OpenGL calls defined in the system headers can be called here; no loaders
//   are used.
// - Functions in this file must be compliant with OpenGL 2.1 and OpenGL ES 2.0
// - Functions must also be complaint with OpenGL 3.0+, unless they are overridden
//   by a function in imgui_image_adv.cpp.

#include "imgui/imgui_image.h"
#include "gl.h"

#ifndef GL_GENERATE_MIPMAP
#define GL_GENERATE_MIPMAP 0x8191
#endif
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2ext.h>
#if !defined(GL_UNPACK_ROW_LENGTH) && defined(GL_UNPACK_ROW_LENGTH_EXT)
#define GL_UNPACK_ROW_LENGTH GL_UNPACK_ROW_LENGTH_EXT
#endif
#endif

int funcGetMaxTextureSize()
{
    int maxTextureSize;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
    return maxTextureSize;
}

unsigned int funcMakeImageTexture()
{
    GLuint gl_text;
    glGenTextures(1, &gl_text);
    return gl_text;
}

void funcUpdateImageTexture(unsigned int gl_text, uint32_t* buffer, int width, int height)
{
    glBindTexture(GL_TEXTURE_2D, gl_text);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void funcUpdateMMImageTexture_GL2(unsigned int gl_text, uint32_t* buffer, int width, int height)
{
#if defined(IMGUI_IMPL_OPENGL_ES2)
    if (width % 2 == 1 || height % 2 == 1)
    {
        funcUpdateImageTexture(gl_text, buffer, width, height);
        return;
    }
    glBindTexture(GL_TEXTURE_2D, gl_text);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
    glGenerateMipmap(GL_TEXTURE_2D);
#else
    glBindTexture(GL_TEXTURE_2D, gl_text);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
#endif
    glBindTexture(GL_TEXTURE_2D, 0);
}

void funcDeleteImageTexture(unsigned int /*gl_text*/)
{
    //glDeleteTextures(1, &gl_text);
}

void bindBaseTextureFunctions()
{
    maxTextureSize = funcGetMaxTextureSize();
    makeImageTexture = funcMakeImageTexture;
    updateImageTexture = funcUpdateImageTexture;
    updateMMImageTexture = funcUpdateMMImageTexture_GL2;
    deleteImageTexture = funcDeleteImageTexture;
}
