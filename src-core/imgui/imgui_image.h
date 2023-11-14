#pragma once

#include "imgui.h"
#include <cstdint>
#include <functional>
#include "dll_export.h"

SATDUMP_DLL extern size_t maxTextureSize;
SATDUMP_DLL extern std::function<unsigned int()> makeImageTexture;
SATDUMP_DLL extern std::function<void(unsigned int, uint32_t *, int, int)> updateImageTexture;
SATDUMP_DLL extern std::function<void(unsigned int, uint32_t *, int, int)> updateMMImageTexture;
SATDUMP_DLL extern std::function<void(unsigned int)> deleteImageTexture;

void ushort_to_rgba(uint16_t *input, uint32_t *output, int size, int channels = 1);

void uchar_to_rgba(uint8_t *input, uint32_t *output, int size, int channels = 1);